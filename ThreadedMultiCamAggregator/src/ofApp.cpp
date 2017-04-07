#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    
    //gui setup
    setupGui();
    
    //setup pixel objects
    
    
    //time of the last save/load event
    //start them out earlier so they dont draw on startup
    lastLoadTime = -10.0f;
    lastSaveTime = -10.0f;
    
    
    
    //----------OSC setup----------
    
    //get the IP/Port from file
    ofBuffer buffer = ofBufferFromFile("osc.txt");
    
    oscIP = "";
    oscPort = 0;
    
    if(buffer.size()) {
        
        int lineNum = 0;
        
        for (ofBuffer::Line it = buffer.getLines().begin(), end = buffer.getLines().end(); it != end; ++it) {
            
            string line = *it;
            
            if(lineNum == 0){
                oscIP = line;
            } else if(lineNum == 1){
                oscPort = ofToInt(line);
            }
            
            lineNum++;
        }
        
    }
    
    osc.setup(oscIP, oscPort);
    lastOSCSendTime = 0;
    
    
    
    
    //Holds which zone is active (-1 if none)
    activeZone = -1;
    
    //detection zone setup
    zonePtRad = 8;
    
    zones.resize(numZones);
    
    for( int i = 0; i < numZones; i++){
        zones[i].setup(i);
        
        //give each zone a reference to the right gui sliders
        switch (i) {
            case 0:
                zones[i].setGuiRefs(&dangerPt0, &dangerPt1, &dangerPt2, &dangerPt3);
                break;
                
            case 1:
                zones[i].setGuiRefs(&active1Pt0, &active1Pt1, &active1Pt2, &active1Pt3);
                break;
                
            case 2:
                zones[i].setGuiRefs(&active2Pt0, &active2Pt1, &active2Pt2, &active2Pt3);
                break;
                
            case 3:
                zones[i].setGuiRefs(&active3Pt0, &active3Pt1, &active3Pt2, &active3Pt3);
                break;
                
            default:
                break;
        }
        
    }
    
    
    //load up the gui settings
    loadSettings();
    
    
    //get gui values and update zone paths
    applyGuiValsToZones();
    
    //-----------------------------------------
    //----------content layout and UI----------
    //-----------------------------------------
    
    //0 = Cams 0-1
    //1 = Cams 2-3
    //2 = Cams 4-5
    //3 = stitching mode view
    //4 = Pipeline
    //5 = zones view
    viewMode = 0;
    currentView = 0;
    
    
    titleFont.load("fonts/Aller_Rg.ttf", 40);
    smallerFont.load("fonts/Aller_Rg.ttf", 16);
    
    //arrange content to start with, will be changed later
    //based on stitched view dimensions
    leftMargin = 250;
    topMargin = 75;
    gutter = 30;
    
    titlePos.set(leftMargin, titleFont.stringHeight("Ag"));
    
    //masterPix object will get immediately re-allocated when things are repositioned,
    //but allocate it much larger to start just to be safe
    masterPix.allocate(camWidth*3, camWidth*3, OF_IMAGE_GRAYSCALE);
    
    masterWidth = camWidth*3;
    masterHeight = camHeight*2;
    
    //keep track of what width and height used to be
    //so we know when it changes
    oldMasterWidth = 0;
    oldMasterHeight = 0;
    
    
    slot1.set(leftMargin, topMargin);
    slot2.set(leftMargin + masterWidth + gutter, topMargin);
    slot3.set(leftMargin, topMargin + masterHeight + gutter);
    slot4.set(leftMargin + masterWidth + gutter, topMargin + masterHeight + gutter);
    slot5.set(leftMargin, topMargin + masterHeight*2 + gutter*2);
    slot5.set(leftMargin + masterWidth + gutter, topMargin + masterHeight*2 + gutter*2);
    
    detectionDisplayPos.set(leftMargin, topMargin + 10);
    compositeDisplayScale = 1.5f;
    pipelineDisplayScale = 1.0f;
    

    //-----------------------------------------
    //-----------------Cameras-----------------
    //-----------------------------------------
    
    //Setup thermal camera framework
    thermal.setup();
    
    //add a listener to the thermal client's new frame event
    ofAddListener( thermal.newFrameEvt, this, &ofApp::addNewFrameToQueue );
    
    
    //setup the individual feed objects
    
    
    vector<int> camIDs = {  0,
                            0,
                            0,
                            0,
                            0,
                            0 };
    
    feeds.resize(6);
    for( int i = 0; i < feeds.size(); i++){
        
        //cam number, USB ID, width and height
        feeds[i].setup( i, camIDs[i], camWidth, camHeight );
        
        //set refs to GUI vals
        feeds[i].contrastExp = &contrastExpSlider;
        feeds[i].contrastPhase = &contrastPhaseSlider;
        feeds[i].stdDevThresh = &stdDevThreshSlider;
        feeds[i].stdDevToggle = &stdDevBlackOutToggle;
    }
    
    

    
    
    
}


void ofApp::applyGuiValsToZones(){
    
    for( int i = 0; i < numZones; i++){
        
        //give gui points to zone depending on which one it is
        switch (i) {
            case 0:
                zones[i].setPoints(dangerPt0, dangerPt1, dangerPt2, dangerPt3);
                break;
                
            case 1:
                zones[i].setPoints(active1Pt0, active1Pt1, active1Pt2, active1Pt3);
                break;
                
            case 2:
                zones[i].setPoints(active2Pt0, active2Pt1, active2Pt2, active2Pt3);
                break;
                
            case 3:
                zones[i].setPoints(active3Pt0, active3Pt1, active3Pt2, active3Pt3);
                break;
                
            default:
                break;
        }
        
        //update to set the points to the path
        zones[i].update();
        
    }
    
}

//--------------------------------------------------------------
void ofApp::addNewFrameToQueue( ofxThermalClient::NewFrameData &nf ){
    frameQueue.push_back(nf);
    
//    cout << "New Frame added to queue from: " << nf.ID << endl;
}


//--------------------------------------------------------------
void ofApp::update(){
    
    //for convenience, set the gui pos value so it stays where we want it
    gui2Pos = gui2.getPosition();
    stitchingGuiPos = stitchingGui.getPosition();
    
    float contentAreaWidth = ofGetWidth() - leftMargin - 20; //make room for margin on the right
    float contentAreaHeight = ofGetHeight() - 250;
    
    //scale for fitting one large composite view on screen
    compositeDisplayScale = contentAreaWidth/(float)masterWidth;
    
    //scale for fitting all the CV pipeline images on screen
    //choose minimum between width and height scale
    pipelineDisplayScale = std::min( contentAreaWidth/(float)( masterWidth * 2 + gutter ), contentAreaHeight/(float)( masterHeight*3 + gutter*2 ) );

    
    
    //Go through the zone points, and assign the point to the mouse
    //if it's being clicked
    adjustedMouse.set( ofGetMouseX() - detectionDisplayPos.x, ofGetMouseY() - detectionDisplayPos.y );
    
    //the camera view is scaled up, so scale the adjusted mouse down
    adjustedMouse /= compositeDisplayScale;
    
    //clamp to cam dimensions
    adjustedMouse.set( ofClamp( adjustedMouse.x, 0, masterWidth),
                      ofClamp( adjustedMouse.y, 0, masterHeight) );
    
    for( int i = 0; i < zones.size(); i++){
        
        zones[i].setClickedPoint( adjustedMouse.x, adjustedMouse.y );
        
    }
    
    
    
    //update the aggregator
    aggregator.update();
    
    
    
    //This method will check for a new frame then trigger
    //an event that will notify the "addFrameToQueue()"
    //method and add the data to the queue
    
    thermal.checkForNewFrame();
    
//    cout << "Queue size: " << frameQueue.size() << endl;
    
    
    //new thermal cam frame?
    if( !frameQueue.empty() ){
        
        
        
        //find which camera the frame is from
        int thisCamId = (*frameQueue.begin()).ID;
        
//        cout << "New frame from: " << thisCamId << endl;
        
        
        //get pix from cam
        ofxCvColorImage rawImg;
        rawImg.allocate(camWidth, camHeight);
        rawImg.setFromPixels( (*frameQueue.begin()).pix.getData() , camWidth, camHeight);

        
        //Now that we've retrieved the data from the queue, get rid of the oldest one.
        //App framerate is much faster than even 3 incoming cameras so no need
        //to process multiple frames in one pass
        frameQueue.pop_front();
        
        
        //convert RGBA camera data to single grayscale ofPixels
        ofxCvGrayscaleImage grayImg;
        grayImg.allocate(camWidth, camHeight);
        grayImg = rawImg;
        
        
        ofPixels raw;
        raw.setFromPixels(grayImg.getPixels().getData(), camWidth, camHeight, 1);
        raw.mirror(false, true);
        
        //blur it
        grayImg.blurGaussian(blurAmountSlider);

        //create a pixel object
        ofPixels gray;
        gray.setFromPixels(grayImg.getPixels().getData(), camWidth, camHeight, 1);
        gray.mirror(false, true);
        
        //put the camera frame into the appropriate pixel object
        //Also flip it (since the camera is mirrored) and adjust contrast
        //since it's new data
        int whichCam = -1;
        
        
        
        //find which cam it belongs to
        for(int i = 0; i < feeds.size(); i++){

            if( testID == thisCamId ){
//            if( feeds[i].camID == thisCamId ){
                whichCam = i;
                
                //send the raw and gray frames into the feed object
                feeds[i].newFrame( raw, gray );
                
//                break;
            }
        }
        

                
        
        if( whichCam == -1 ){
            cout << "ID not recognized: " << thisCamId <<endl;
        }
        
        
        

        
        //get the furthest Right and down parts of the aggregated image
        //so the masterPix object can be resized appropriately
        int furthestRight = 0;
        int furthestDown = 0;
        int furthestLeft = 10000;
        int furthestUp = 10000;
        
        //put the camera positons into a vector for convenience
        for(int i = 0; i < NUM_CAMS; i++){
            
            //dimensions will changed if camera is pasted into
            //master pix straight or rotated 90
            int xDim = camRotations[i] % 2 == 1 ? camHeight : camWidth;
            int yDim = camRotations[i] % 2 == 1 ? camWidth : camHeight;

            
            int thisMaxRight = camPositions[i] -> x + xDim;
            if(thisMaxRight > furthestRight){
                furthestRight = thisMaxRight;
            }
            
            int thisMaxBottom = camPositions[i] -> y + yDim;
            if(thisMaxBottom > furthestDown){
                furthestDown = thisMaxBottom;
            }
            
            if(camPositions[i] -> x < furthestLeft) furthestLeft = camPositions[i] -> x;
            if(camPositions[i] -> y < furthestUp) furthestUp = camPositions[i] -> y;
            
        }
        
        
        
        //trim out the space above and to the left in masterPix
        if( (furthestLeft > 0 || furthestUp > 0) && trimMasterPixButton ){
            
            //subtract from all the camera position gui values
//            cam1Pos = oldGuiVal - ofVec2f( furthestLeft, furthestUp );
//            cam2Pos = oldGuiVal - ofVec2f( furthestLeft, furthestUp );
//            cam3Pos = oldGuiVal - ofVec2f( furthestLeft, furthestUp );
            
            for( int i = 0; i < NUM_CAMS; i++){
                ofVec2f oldGuiVal = camPositions[i];
                camPositions[i] = oldGuiVal - ofVec2f( furthestLeft, furthestUp );
            }
            
            //subtract the trim amount from the master dimensions too
            //so that the masterPix object doesnt have to wait for the next
            //frame to update the trim
            masterWidth -= furthestLeft;
            masterHeight -= furthestUp;
            
        }
        
        
        masterWidth = furthestRight;
        masterHeight = furthestDown;
        

        
        //if any of the dimensions are different, we need to reallocate
        if( oldMasterWidth != masterWidth || oldMasterHeight != masterHeight ){

            //Prepare the masterPix object to receive the camera's pixels
            masterPix.clear();
            
            //Set the masterPix object to the new min and max bounds
            masterPix.allocate(masterWidth, masterHeight, OF_IMAGE_GRAYSCALE);
            masterPix.setColor(ofColor(0));
            
            
            //reallocate all the other pixel objects while we're at it
            processedPix.clear();
            processedPix.allocate(masterWidth, masterHeight, 1);
            
            threshPix.clear();
            threshPix.allocate(masterWidth, masterHeight, 1);
            
            backgroundPix.clear();
            backgroundPix.allocate(masterWidth, masterHeight, 1);
            
            foregroundPix.clear();
            foregroundPix.allocate(masterWidth, masterHeight, 1);

            //also tell the background to reset
            bNeedBGReset = true;
            
            
            //now store the new dims as old ones
            oldMasterHeight = masterHeight;
            oldMasterWidth = masterWidth;
            
            cout << "Re-allocating pixel objects" << endl;
        
        }

        
        //all feeds are blended into masterPix object so we need to
        //clear it every frame to avoid pixel build up
        masterPix.setColor(ofColor(0));
        
        
        //Now paste the new frame into the masterPix object
        for (int i = 0; i < NUM_CAMS; i++){
            
            ofPixels feedOutput = feeds[i].getOutputPix();
            if( camRotations[i] != 0 ){
                feedOutput.rotate90( camRotations[i] );
            }
            feedOutput.blendInto(masterPix, camPositions[i] -> x, camPositions[i] -> y);
            
        }
        
        
        //Contrast/bluring already done before Master pix,
        //so paste master directly into processed
        //(This could be optimized away)
        processedPix = masterPix;
        
        
        
        
        
        //threshold if we're not using the running background
        //otherwise, ofxCv::RunningBackground already returns a thresholded image
        
        //also, don't use BG diff on the stitching screen
        if( !useBgDiff ){
            
            ofxCv::threshold(processedPix, threshPix, thresholdSlider);
            
            //set flag to true in case we switch back to using BG diff again
            bNeedBGReset = true;
            
            //fill the back/foreground objects with something
            //to clear buffer garbage being drawn to screen
            backgroundPix.setColor(70);
            foregroundPix.setColor(70);
            
        } else {
            
            if(bNeedBGReset || resetBGButton){
                
                background.reset();
                bNeedBGReset = false;
                
            }
            
            background.setDifferenceMode(ofxCv::RunningBackground::BRIGHTER);
            background.setLearningTime(learningTime);
            background.setThresholdValue(thresholdSlider);
            
            background.update(processedPix, threshPix);
            
            //get the foreground/foreground to draw to screen
            ofxCv::toOf( background.getBackground(), backgroundPix );
            ofxCv::toOf( background.getForeground(), foregroundPix );
        }
        
        
        //ERODE it
        for(int i = 0; i < numErosionsSlider; i++){
            ofxCv::erode(threshPix);
        }
        
        //DILATE it
        for(int i = 0; i < numDilationsSlider; i++){
            ofxCv::dilate(threshPix);
        }
        
        
        //Define contour finder
        contours.setMinArea(minBlobAreaSlider);
        contours.setMaxArea(maxBlobAreaSlider);
        contours.setThreshold(254);  //only detect white
        
        // wait before forgetting something
        contours.getTracker().setPersistence(persistenceSlider);
        
        // an object can move up to X pixels per frame
        contours.getTracker().setMaximumDistance(maxDistanceSlider);
        
        //find dem blobs
        contours.findContours(threshPix);
        
        
        //Go through contours and see if any of the points lie within the detection zones
        //start from inside and work outward. If inner zones are triggered, no need to check outerzone
        
        bool foundObject = false;
        activeZone = -1;
        
        for(int i = 0; i < zones.size(); i++){
            
            //get the ofPolyline from the zone's internal ofPath so we
            //can use .inside()
            ofPolyline p = zones[i].path.getOutline()[0];
            
            //for each CONTOUR...
            for(int j = 0; j < contours.size(); j++){
                
                ofPolyline points = contours.getPolyline(j).getVertices();
                
                //for each POINT...
                for(int k = 0; k < points.size(); k++){
                    
                    if( p.inside(points[k]) ){
                        foundObject = true;
                        activeZone = i;
                        
                        //no need to check other points in this contour
                        break;
                    }
                    
                }
                
                //no need to check other contours
                if( foundObject ){
                    break;
                }
                
            }
            
            //no need to check other zones
            if( foundObject ){
                break;
            }
        }
        
        
        
        
        //if we found something, send the OSC message
        if( foundObject ){
            
            ofxOscMessage m;
            
            m.setAddress("/Detected");
            m.addIntArg(activeZone);
            
            osc.sendMessage(m);
            
            lastOSCSendTime = ofGetElapsedTimef();
            
        }
        
        
        
        
        
    }
    
    //    cout << "Detection bools: " << bInZone[0] << ", " << bInZone[1] << ", " << bInZone[2] << ", " << bInZone[3] << endl;
    
    
    
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofBackgroundGradient(100, 0);
    
    
    ofSetColor(255);
    ofDrawBitmapString("Framerate: " + ofToString(ofGetFrameRate()), 10, 15);

    
    
    string title = "";
    

    
    if( currentView >= 0 && currentView <=2 ){
        
        //----------CAMERAS VIEW----------
        int first = currentView * 2;
        int second = first + 1;
        
        title = "Cameras " +ofToString(first)+ " & " +ofToString(second);

        slot1.set(leftMargin, topMargin);
        slot2.set(leftMargin + camWidth*2 + gutter*2, topMargin);
        
        feeds[first].draw(slot1.x, slot1.y);
        feeds[second].draw(slot2.x, slot2.y);
        
        //----------pixel distribution statistical info----------
        feeds[first].pixelStats.drawDistribution(slot1.x, slot1.y + 200, camWidth*2, 200);
        feeds[second].pixelStats.drawDistribution(slot2.x, slot2.y + 200, camWidth*2, 200);
        

        
    } else if( currentView == 3 ){
        
        
        //----------STITCHING VIEW----------
        
        title = "Stitching/Composite View";

        //cam repositioning warning
        ofSetColor(255, 0, 0);
        smallerFont.drawString("NOTE:", leftMargin, detectionDisplayPos.y + compositeDisplayScale*masterHeight + smallerFont.stringHeight("Ag")*2);
        
        ofSetColor(255);
        string s = "";
        s += "Learned background subtraction is cleared whenever the\n";
        s += "composite image's dimensions are changed.";
        
        smallerFont.drawString(s, leftMargin, detectionDisplayPos.y + compositeDisplayScale*masterHeight + smallerFont.stringHeight("Ag")*3);
        
        ofPushMatrix();{
            
            ofTranslate(detectionDisplayPos);
            ofScale(compositeDisplayScale, compositeDisplayScale);
            
            ofImage img;
            
            ofSetColor(255);
            ofSetLineWidth(1);
            img.setFromPixels(masterPix.getData(), masterWidth, masterHeight, OF_IMAGE_GRAYSCALE);
            img.draw(0, 0);
            
            ofNoFill();
            ofDrawRectangle(0, 0, masterWidth, masterHeight);
            
            //draw individual outlines of individual cameras and titles
            for( int i = 0; i < NUM_CAMS; i++){
                
                ofColor c;
                c.setHsb( i * 255/NUM_CAMS, 200, 200);
                
                ofSetColor(c);
                ofDrawBitmapString("Cam " + ofToString(i), camPositions[i] -> x + 5, camPositions[i] -> y + 10);
                ofDrawRectangle(camPositions[i] -> x, camPositions[i] -> y, camRotations[i] % 2 == 1 ? camHeight : camWidth, camRotations[i] % 2 == 1 ? camWidth : camHeight);
                
                
            }
            
            
//
//            ofSetColor(0, 128, 255);
//            ofDrawBitmapString("Cam 2", cam2Pos -> x + 5, cam2Pos -> y + 10);
//            ofDrawRectangle(cam2Pos -> x, cam2Pos -> y, cam2Rotate90Slider % 2 == 1 ? camHeight : camWidth, cam2Rotate90Slider % 2 == 1 ? camWidth : camHeight);
//
//            ofSetColor(200, 0, 255);
//            ofDrawBitmapString("Cam 3", cam3Pos -> x + 5, cam3Pos -> y + 10);
//            ofDrawRectangle(cam3Pos -> x, cam3Pos -> y, cam3Rotate90Slider % 2 == 1 ? camHeight : camWidth, cam3Rotate90Slider % 2 == 1 ? camWidth : camHeight);
            
            
        }ofPopMatrix();
        

        
        
        
        
        
        
    } else if( currentView == 4 ){
        
        //----------PIPELINE VIEW----------
        title = "CV Pipeline";
        
        //adjust the content positions to reflect the changed position of the
        //composite image
        slot1.set(0                   , 0);
        slot2.set(masterWidth + gutter, 0);
        slot3.set(0                   , masterHeight + gutter);
        slot4.set(masterWidth + gutter, masterHeight + gutter);
        slot5.set(0                   , masterHeight*2 + gutter*2);
        slot6.set(masterWidth + gutter, masterHeight*2 + gutter*2);
        
        ofPushMatrix();{
            
            ofTranslate(leftMargin, topMargin);
            ofScale(pipelineDisplayScale, pipelineDisplayScale);

            ofImage img;
            
            //----------slot 2----------
            //empty
            
            //----------slot 3----------
            ofSetColor(255);
            ofDrawBitmapString("Stitched & Processed", slot3.x, slot3.y - 5);
            img.setFromPixels(processedPix.getData(), processedPix.getWidth(), processedPix.getHeight(), OF_IMAGE_GRAYSCALE);
            img.draw(slot3);
            
            ofNoFill();
            ofDrawRectangle(slot3, masterWidth, masterHeight);
            
            //----------slot 4----------
            ofSetColor(255);
            ofDrawBitmapString("Subtracted Background", slot4.x, slot4.y - 5);
            img.setFromPixels(backgroundPix.getData(), backgroundPix.getWidth(), backgroundPix.getHeight(), OF_IMAGE_GRAYSCALE);
            img.draw(slot4);
            
            ofNoFill();
            ofDrawRectangle(slot4, masterWidth, masterHeight);
            
            //----------slot 5----------
            ofDrawBitmapString("Foreground", slot5.x, slot5.y - 5);
            img.setFromPixels(foregroundPix.getData(), foregroundPix.getWidth(), foregroundPix.getHeight(), OF_IMAGE_GRAYSCALE);
            img.draw(slot5);
            
            ofNoFill();
            ofDrawRectangle(slot5, masterWidth, masterHeight);
            
            //----------slot 6----------
            ofDrawBitmapString("Thresholded", slot6.x, slot6.y - 5);
            img.setFromPixels(threshPix.getData(), threshPix.getWidth(), threshPix.getHeight(), OF_IMAGE_GRAYSCALE);
            img.draw(slot6);
            
            ofNoFill();
            ofDrawRectangle(slot6, masterWidth, masterHeight);
            
            

            
        }ofPopMatrix();
        
        //draw contours over thresholded image in slot 5
        if(drawContoursToggle){
            
            ofPushMatrix();
            ofTranslate(leftMargin + slot6.x*pipelineDisplayScale, topMargin + slot6.y*pipelineDisplayScale);
            ofScale(pipelineDisplayScale, pipelineDisplayScale);
            
            //draw contours
            ofSetLineWidth(2.0);
            ofSetColor(255, 0, 0);
            contours.draw();
            
            
            //go through and draw blob data too
            for(int i = 0; i < contours.size(); i++) {
                
                int label = contours.getLabel(i);
                ofPoint c = ofxCv::toOf(contours.getCenter(i));
                
                ofFill();
                ofSetColor(0, 180, 0);
                ofDrawCircle(c.x, c.y, 5, 5);
                
                if(showInfoToggle){
                    string msg = ofToString(label) + " : " + ofToString(c.x) + ", " + ofToString(c.y);
                    ofDrawBitmapStringHighlight(msg, c.x + 5, c.y + 10);
                }
            }
            
            ofPopMatrix();
            
        }

        
        

        
        
                
        
    } else if( currentView == 5 ){
        
        //----------DETECTION ZONE VIEW----------
        title = "Detection Zones";
        
        
        //----------Primary Slot----------
        ofDrawBitmapString("Contours and Detection zones", detectionDisplayPos.x, detectionDisplayPos.y - 5);
        
        ofPushMatrix();{
            
            ofTranslate(detectionDisplayPos);
            ofScale(compositeDisplayScale, compositeDisplayScale);
            
            ofSetColor(0, 255);
            ofFill();
            ofDrawRectangle(0, 0, masterWidth, masterHeight);
            ofImage img;
            img.setFromPixels(threshPix.getData(), threshPix.getWidth(), threshPix.getHeight(), OF_IMAGE_GRAYSCALE);
            ofSetColor(255, 100);
            img.draw(0, 0);
            
            
            //draw border
            ofSetColor(255);
            ofSetLineWidth(1);
            ofNoFill();
            ofDrawRectangle(0, 0, masterWidth, masterHeight);
            
            if(drawContoursToggle){
                
                //draw contours
                ofSetLineWidth(2.0);
                ofSetColor(255, 0, 0);
                contours.draw();
                
                
                //go through and draw blob data too
                for(int i = 0; i < contours.size(); i++) {
                    
                    int label = contours.getLabel(i);
                    ofPoint c = ofxCv::toOf(contours.getCenter(i));
                    
                    ofFill();
                    ofSetColor(0, 180, 0);
                    ofDrawCircle(c.x, c.y, 5, 5);
                    
                    if(showInfoToggle){
                        string msg = ofToString(label) + " : " + ofToString(c.x) + ", " + ofToString(c.y);
                        ofDrawBitmapStringHighlight(msg, c.x + 5, c.y + 10);
                    }
                }
                
            }
            
            
            
            
            
            //draw detection zones
            
            ofPushStyle();{
                
                //draw a white slice for the active zone
                if( activeZone != -1 ){
                    
                    ofPath slice = zones[ activeZone ].path;
                    slice.setFilled(true);
                    ofColor c(zones[activeZone].col, 150);
                    slice.setFillColor( c );
                    
                    //if we're not zone 0, subtract from it the next closest zone
                    if( activeZone > 0 ){
                        slice.close();
                        slice.append(zones[activeZone - 1].path);
                    }
                    
                    slice.draw();
                    
                }
                
                
                for(int i = zones.size() - 1; i >= 0 ; i--){
                    zones[i].draw();
                }
                
                
                
                
            }ofPopStyle();
            
            
        }ofPopMatrix();
        
        //OSC status and info
        
        string oscData = "";
        
        oscData += "OSC Info\n";
        oscData += "------------------\n";
        oscData += "Destination IP:\n";
        oscData += oscIP + "\n";
        oscData += "Destination PORT:\n";
        oscData += ofToString(oscPort) + "\n";
        
        
        ofSetColor(255);
        ofDrawBitmapString(oscData, detectionDisplayPos.x, detectionDisplayPos.y + ( masterHeight * compositeDisplayScale) + 30);
        
        string sentString = "OSC MESSAGE SENT...";
        float t = ofMap(ofGetElapsedTimef() - lastOSCSendTime, 0, 0.1, 255, 100, true);
        ofSetColor(255, 0, 0, t);
        
        ofDrawBitmapString(sentString, detectionDisplayPos.x, detectionDisplayPos.y + ( masterHeight * compositeDisplayScale) + 120);
        
        
        
    }
    
    ofSetColor(255);
    titleFont.drawString(title, titlePos.x, titlePos.y);
    
    
    
    string keyInfo = "";
    
    keyInfo += "Key Bindings\n";
    keyInfo += "------------\n";
    keyInfo += "'S' to Save settings\n";
    keyInfo += "'L' to Load settings\n";
    keyInfo += "Left/Right to switch\n";
    keyInfo += "between views:\n";
    keyInfo += "-Camera Stitching\n";
    keyInfo += "-CV Pipeline\n";
    keyInfo += "-Detection Zones\n";
    
    ofDrawBitmapString(keyInfo, 10, ofGetHeight() - 135);
    
    drawGui(10, 40);
    
    
    
    //box with save/load feedback
    drawSaveLoadBox();
    
    
}




//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    if(key == 's'){
        saveSettings();
        lastSaveTime = ofGetElapsedTimef();
    }
    
    if(key == 'l'){
        loadSettings();
        lastLoadTime = ofGetElapsedTimef();
    }
    
    if( key == OF_KEY_LEFT ){
        currentView--;
        if(currentView < 0) currentView = numViews - 1;
    }
    
    if( key == OF_KEY_RIGHT || key == ' ' ){
        currentView++;
        if(currentView > numViews - 1) currentView = 0;
    }
    
    
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
    if( currentView == 5 ){
        
        //only check if the mouse is in the primary slot
        if( x > detectionDisplayPos.x && x < detectionDisplayPos.x + masterWidth * compositeDisplayScale && y > detectionDisplayPos.y && y < detectionDisplayPos.y + masterHeight * compositeDisplayScale){
            
            for( int i = 0; i < zones.size(); i++){
                
                //check for points, if one is found, stop looking
                //Also, adjust mouse to acount for frame position on screen
                if( zones[i].checkForClicks(adjustedMouse.x, adjustedMouse.y) ){
                    break;
                }
            }
            
        }
    }
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
    //set all points to released on mouse up
    for( int i = 0; i < zones.size(); i++){
        zones[i].releasePoints();
    }
    
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}

void ofApp::setupGui(){
    
    guiName = "settings";
    
    gui.setup(guiName, guiName + ".xml", 0, 0);
    
    gui.add(imageAdjustLabel.setup("   IMAGE ADJUSTMENT", ""));
    gui.add(blurAmountSlider.setup("Blur", 1, 0, 40));
    gui.add(contrastExpSlider.setup("Contrast Exponent", 1.0, 1.0, 8.0));
    gui.add(contrastPhaseSlider.setup("Contrast Phase", 0.0, 0.0, 0.4));
    gui.add(stdDevBlackOutToggle.setup("Use Std Dev Blackout", false));
    gui.add(stdDevThreshSlider.setup("Std Dev Thresh", 300, 0, 1000));
    gui.add(thresholdSlider.setup("Threshold", 0, 0, 255));
    gui.add(numErosionsSlider.setup("Number of erosions", 0, 0, 10));
    gui.add(numDilationsSlider.setup("Number of dilations", 0, 0, 10));
    
    gui.add(bgDiffLabel.setup("   BG SUBTRACTION", ""));
    gui.add(useBgDiff.setup("Use BG Diff", false));
    gui.add(learningTime.setup("Frames to learn BG", 100, 0, 300));
    gui.add(resetBGButton.setup("Reset Background"));
    
    gui.add(contoursLabel.setup("   CONTOUR FINDING", ""));
    gui.add(minBlobAreaSlider.setup("Min Blob Area", 0, 0, 1000));
    gui.add(maxBlobAreaSlider.setup("Max Blob Area", 1000, 0, 20000));
    gui.add(drawContoursToggle.setup("Draw Contours", true));
    gui.add(showInfoToggle.setup("Info", false));
    
    
    
    //make much bigger to accomodate
    //for different stitching layouts
    ofVec2f start(0, 0);
    ofVec2f end(camWidth*4, camHeight*4);
    
    
    
    gui.add(detectionLabel.setup("   DETECTION ZONES", ""));
    gui.add(showSecondGui.setup("Show Control Points", false));
    
    
    
    gui.minimizeAll();
    
    
    
    gui2Name = "controlPoints";
    gui2.setup(gui2Name, gui2Name + ".xml", 0, 0);
    gui2.add( gui2Pos.setup("Gui Pos", ofVec2f(200, 50), ofVec2f(0, 0), ofVec2f(ofGetWidth(), ofGetHeight())));
    
    
    gui2.add(zonePointsLabel.setup("   DETECTION ZONE POINTS", ""));
    gui2.add( dangerPt0.setup("Danger Z. Pt 0", start, start, end));
    gui2.add( dangerPt1.setup("Danger Z. Pt 1", start, start, end));
    gui2.add( dangerPt2.setup("Danger Z. Pt 2", start, start, end));
    gui2.add( dangerPt3.setup("Danger Z. Pt 3", start, start, end));
    gui2.add(active1Pt0.setup("Active Z-1 Pt 0", start, start, end));
    gui2.add(active1Pt1.setup("Active Z-1 Pt 1", start, start, end));
    gui2.add(active1Pt2.setup("Active Z-1 Pt 2", start, start, end));
    gui2.add(active1Pt3.setup("Active Z-1 Pt 3", start, start, end));
    gui2.add(active2Pt0.setup("Active Z-2 Pt 0", start, start, end));
    gui2.add(active2Pt1.setup("Active Z-2 Pt 1", start, start, end));
    gui2.add(active2Pt2.setup("Active Z-2 Pt 2", start, start, end));
    gui2.add(active2Pt3.setup("Active Z-2 Pt 3", start, start, end));
    gui2.add(active3Pt0.setup("Active Z-3 Pt 0", start, start, end));
    gui2.add(active3Pt1.setup("Active Z-3 Pt 1", start, start, end));
    gui2.add(active3Pt2.setup("Active Z-3 Pt 2", start, start, end));
    gui2.add(active3Pt3.setup("Active Z-3 Pt 3", start, start, end));
    
    //    gui2.add(stitchingPointsLabel.setup("   STITCHING POSITIONS", ""));
    
    
    gui2.minimizeAll();
    
    stitchingGuiName = "stitchingGui";
    stitchingGui.setup(stitchingGuiName, stitchingGuiName + ".xml", 0, 0);
    stitchingGui.add( stitchingGuiPos.setup("Gui Pos", ofVec2f(200, 50), ofVec2f(0, 0), ofVec2f(ofGetWidth(), ofGetHeight())));
    stitchingGui.add(stitchingLabel.setup("   CAMERA STITCHING", ""));
    
    for(int i = 0; i < NUM_CAMS; i++){
        stitchingGui.add(camRotations[i].setup("Cam " +ofToString(i)+ " Rotations", 0, 0, 3));
    }
    
    start.set(0, 0);
    end.set(camWidth*3, camHeight);

    for(int i = 0; i < NUM_CAMS; i++){
        stitchingGui.add(camPositions[i].setup("Cam " +ofToString(i)+ " Position", ofVec2f(0, 0), start, end));
    }
    
    stitchingGui.add(trimMasterPixButton.setup("Trim pixels"));

    stitchingGui.minimizeAll();
    
    //-----GUI 1 formatting-----
    gui.setHeaderBackgroundColor(ofColor(255));
    
    //color applies to gui title only
    gui.setTextColor(ofColor(0));
    
    imageAdjustLabel.setBackgroundColor(ofColor(255));
    bgDiffLabel.setBackgroundColor(ofColor(255));
    contoursLabel.setBackgroundColor(ofColor(255));
    detectionLabel.setBackgroundColor(ofColor(255));
    
    //this changes the color of all the labels
    contoursLabel.setDefaultTextColor(ofColor(0));
    
    
    //-----GUI 2 formatting-----
    gui2.setHeaderBackgroundColor(ofColor(255));
    
    //color applies to gui title only
    gui2.setTextColor(ofColor(0));
    
    zonePointsLabel.setBackgroundColor(ofColor(255));
    
    
    //-----Stitching GUI formatting-----
    stitchingGui.setHeaderBackgroundColor(ofColor(255));
    
    //color applies to gui title only
    stitchingGui.setTextColor(ofColor(0));
    stitchingLabel.setBackgroundColor(ofColor(255));
    
}

void ofApp::loadSettings(){
    
    gui.loadFromFile(guiName + ".xml");
    gui2.loadFromFile(gui2Name + ".xml");
    
    gui2.setPosition(gui2Pos -> x, gui2Pos -> y);
    applyGuiValsToZones();
    
    stitchingGui.loadFromFile(stitchingGuiName + ".xml");
    stitchingGui.setPosition(stitchingGuiPos -> x, stitchingGuiPos -> y);
}

void ofApp::saveSettings(){
    
    gui.saveToFile(guiName + ".xml");
    gui2.saveToFile(gui2Name + ".xml");
    stitchingGui.saveToFile(stitchingGuiName + ".xml");
}

void ofApp::drawGui(int x, int y){
    
    gui.setPosition(x, y);
    gui.draw();
    
    if( showSecondGui ){
        gui2.setPosition(gui2Pos -> x, gui2Pos -> y);
        gui2.draw();
    }
    
    //if we're in stitching view
    if(currentView == 3){
        stitchingGui.setPosition(stitchingGuiPos -> x, stitchingGuiPos -> y);
        stitchingGui.draw();
    }
    
    
}




void ofApp::drawSaveLoadBox(){
    
    float stripHeight = 30;
    
    ofFill();
    
    if(ofGetElapsedTimef() - lastSaveTime < 1.0f){
        ofSetColor(0, 180, 0);
        ofDrawRectangle(0, ofGetHeight()/2 - stripHeight/2, ofGetWidth(), stripHeight);
        
        ofSetColor(255);
        ofDrawBitmapString("Settings Saved", ofGetWidth()/2 - 80, ofGetHeight()/2 + 5);
    }
    
    if(ofGetElapsedTimef() - lastLoadTime < 1.0f){
        ofSetColor(0, 128, 255);
        ofDrawRectangle(0, ofGetHeight()/2 - stripHeight/2, ofGetWidth(), stripHeight);
        
        ofSetColor(255);
        ofDrawBitmapString("Settings loaded", ofGetWidth()/2 - 80, ofGetHeight()/2 + 5);
    }
    
}

void ofApp::adjustContrast( ofPixels *pix, float exp, float phase){
    
    for(int i = 0; i < pix -> getWidth() * pix -> getHeight(); i++){
        //normalized pixel value
        float normPixVal = (*pix)[i]/255.0f;
        (*pix)[i] = ofClamp( 255 * pow((normPixVal + phase), exp), 0, 255);
    }
    
}
