#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    
    //gui setup
    setupGui();
    
    //setup pixel objects
    rawGrayPix1.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    rawGrayPix2.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    rawGrayPix3.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    
    processedPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    threshPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    backgroundPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    foregroundPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    
    
    //masterPix object will get immediately re-allocated when things are repositioned,
    //but allocate it much larger to start just to be safe
    masterPix.allocate(camWidth*3, camWidth*3, OF_IMAGE_GRAYSCALE);
    
    //camera
    thermal.setup();
    
    
    //time of the last save/load event
    //start them out earlier so they dont draw on startup
    lastLoadTime = -10.0f;
    lastSaveTime = -10.0f;
    
    
    
    //OSC setup
    
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
    
    
    lastFrameTime = 0;
    lastFrameRate = 0;
    camFrameRate = 0;
    
    frameBlackOut = false;
    
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
    
    
    //content layout
    //0 = Stitching/Placement
    //1 = CV pipeline
    //2 = Detection Zones
    viewMode = 0;
    currentView = 0;
    
    
    font.load("fonts/Aller_Rg.ttf", 40);
    
    //arrange content to start with, will be changed later
    //based on stitched view dimensions
    leftMargin = 250;
    topMargin = 75;
    gutter = 30;
    
    titlePos.set(leftMargin, font.stringHeight("Ag"));
    
    
    slot1.set(leftMargin, topMargin);
    slot2.set(leftMargin + masterWidth + gutter, topMargin);
    slot3.set(leftMargin, topMargin + masterHeight + gutter);
    slot4.set(leftMargin + masterWidth + gutter, topMargin + masterHeight + gutter);
    slot5.set(leftMargin, topMargin + masterHeight*2 + gutter*2);
    slot5.set(leftMargin + masterWidth + gutter, topMargin + masterHeight*2 + gutter*2);
    
    detectionDisplayPos.set(leftMargin, topMargin + 100);
    compositeDisplayScale = 1.5f;
    pipelineDisplayScale = 1.0f;
    
    
    
    //make a bin for each possible pixel value
    //then we'll draw a bar chart later to visualize the
    //statistical break down
    pixelBins.assign(256, 0);
    
    varianceBins.assign(256, 0);
    
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
void ofApp::update(){
    
    //for convenience, set the gui pos value so it stays where we want it
    gui2Pos = gui2.getPosition();
    
    
    float contentAreaWidth = ofGetWidth() - leftMargin - 20; //make room for margin on the right
    
    //scale for fitting one large composite view on screen
    compositeDisplayScale = contentAreaWidth/(float)masterWidth;
    
    //scale for fitting all the CV pipeline images on screen
    pipelineDisplayScale = contentAreaWidth/(float)( masterWidth * 2 + gutter );
    
    
    
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
    
    
    
    
    thermal.checkForNewFrame();
    
    
    //new thermal cam frame?
    if(thermal.receivedNewFrame){
        
        
        //log the frame times (and the second to last one to
        //average it out and smooth the value)
        float thisFrameRate = 1.0/( (ofGetElapsedTimef() - lastFrameTime) );
        
        //average this framerate with the last one to smooth out numbers
        //and get a better reading.
        camFrameRate = (thisFrameRate + lastFrameRate)/2;
        lastFrameRate = thisFrameRate;
        lastFrameTime = ofGetElapsedTimef();
        
        
        
        
        //get pix from cam
        ofxCvColorImage rawImg;
        rawImg.allocate(camWidth, camHeight);
        rawImg.setFromPixels(thermal.getPixels(), camWidth, camHeight);
        
        //convert RGBA camera data to single grayscale ofPixels
        ofxCvGrayscaleImage grayImg;
        grayImg.allocate(camWidth, camHeight);
        grayImg = rawImg;
        
        
        //find which camera the frame is from
        int thisCamId = thermal.getDeviceLocation();
        
        cout << "[ofApp] Device Location: " << thisCamId << endl;
        
        //put the camera frame into the appropriate pixel object
        if( thisCamId == cam1Id ){
            
            grayImg.getPixels().pasteInto(rawGrayPix1, 0, 0);
            rawGrayPix1.mirror(false, true);
            
            grayImg.getPixels().pasteInto(rawGrayPix2, 0, 0);
            rawGrayPix2.mirror(false, true);
            
            grayImg.getPixels().pasteInto(rawGrayPix3, 0, 0);
            rawGrayPix3.mirror(false, true);
        } else if( thisCamId == cam2Id ){
        

        } else if( thisCamId == cam3Id ){
            
        }
        
        
        
        
        
        //Prepare the masterPix object to receive the camera's pixels
        masterPix.clear();
        
        //get the furthest Right and down parts of the aggregated image
        //so the masterPix object can be resized appropriately
        int furthestRight = 0;
        int furthestDown = 0;
        int furthestLeft = 10000;
        int furthestUp = 10000;
        
        //put the camera positons into a vector for convenience
        vector<ofVec2f> positions = { cam1Pos, cam2Pos, cam3Pos };
        
        for(int i = 0; i < positions.size(); i++){
            
            int thisMaxRight = positions[i].x + camWidth;
            if(thisMaxRight > furthestRight){
                furthestRight = thisMaxRight;
            }
            
            int thisMaxBottom = positions[i].y + camHeight;
            if(thisMaxBottom > furthestDown){
                furthestDown = thisMaxBottom;
            }
            
            if(positions[i].x < furthestLeft) furthestLeft = positions[i].x;
            if(positions[i].y < furthestUp) furthestUp = positions[i].y;
            
        }
        
        
        
        //trim out the space above and to the left in masterPix
        if( (furthestLeft > 0 || furthestUp > 0) && trimMasterPixButton ){
            
            //subtract from all the camera position gui values
            ofVec2f oldGuiVal = cam1Pos;
            cam1Pos = oldGuiVal - ofVec2f( furthestLeft, furthestUp );
            
            oldGuiVal = cam2Pos;
            cam2Pos = oldGuiVal - ofVec2f( furthestLeft, furthestUp );
            
            oldGuiVal = cam3Pos;
            cam3Pos = oldGuiVal - ofVec2f( furthestLeft, furthestUp );
            
            //subtract the trim amount from the master dimensions too
            //so that the masterPix object doesnt have to wait for the next
            //frame to update the trim
            masterWidth -= furthestLeft;
            masterHeight -= furthestUp;
            
        }
        
        masterWidth = furthestRight;
        masterHeight = furthestDown;
        
        
        
        //find the min and max bounds depending on the camera positions
        masterPix.allocate(masterWidth, masterHeight, 1);
        masterPix.setColor(ofColor(0));
        
        
        
        //-----OPTIMIZATION NEEDED BELOW!!!-----
        //No need to reallocate things if masterPix doesn't change dimensions
        //i.e. if we're not moving around the camera positions
        
        //reallocate all the other pixel objects while we're at it
        processedPix.clear();
        processedPix.allocate(masterWidth, masterHeight, 1);
        
        threshPix.clear();
        threshPix.allocate(masterWidth, masterHeight, 1);
        
        backgroundPix.clear();
        backgroundPix.allocate(masterWidth, masterHeight, 1);
        
        foregroundPix.clear();
        foregroundPix.allocate(masterWidth, masterHeight, 1);
        
        
        
        //Now paste the new frame into the masterPix object
        rawGrayPix1.blendInto(masterPix, cam1Pos -> x, cam1Pos -> y);
        rawGrayPix2.blendInto(masterPix, cam2Pos -> x, cam2Pos -> y);
        rawGrayPix3.blendInto(masterPix, cam3Pos -> x, cam3Pos -> y);
        
        
        
        //blur (need raw pixels from grayImg first to use ofxCv method)
        ofxCv::GaussianBlur(masterPix, processedPix, blurAmountSlider);
        
        
        //Adjust contrast
        for(int i = 0; i < processedPix.getWidth() * processedPix.getHeight(); i++){
            
            //normalized pixel value
            float normPixVal = processedPix[i]/255.0f;
            //
            processedPix[i] = ofClamp( 255 * pow((normPixVal + contrastPhaseSlider), contrastExpSlider), 0, 255);
        }
        
        
        
        
        
        //threshold if we're not using the running background
        //otherwise, ofxCv::RunningBackground already returns a thresholded image
        
        if( !useBgDiff ){
            
            ofxCv::threshold(processedPix, threshPix, thresholdSlider);
            
            //set flag to true in case we switch back to using BG diff again
            bNeedBGReset = true;
            
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
        
        
        //if we're using the blackout method, find the average pixel
        //and if greater than threshold, feed a blacked out ofPixels instance
        //into the contour finder, but keep the original threshPix untouched
        //for drawing to screen
        frameBlackOut = false;
        
        //needs to be in current scope, but no need to allocate/fill
        //unless we're using the blackout method below
        ofPixels blackOutPix;
        
        pixelAverage = 0;
        stdDev = 0;
        int numSamples = 0;
        
        //fill the pixel val vector with 0's
        std::fill( pixelBins.begin(), pixelBins.end(), 0 );
        std::fill( varianceBins.begin(), varianceBins.end(), 0 );
        
        for(int i = 0; i < masterPix.getWidth() * masterPix.getHeight(); i++){
            
            pixelAverage += masterPix[i];
            numSamples++;
            
            //add one to each bin depending on the pixel value
            pixelBins[ (int)masterPix[i] ] += 1;
            
        }
        
        pixelAverage /= numSamples;
        
        
        //we need the average number of pixels per bin to get the variance
        int avgPixPerBin = 0;
        
        for(int i = 0; i < pixelBins.size(); i++ ){
            avgPixPerBin += pixelBins[i];
        }
        
        avgPixPerBin /= pixelBins.size();
        
        
        //calculate the standard deviation of the pixelBins vector
        //first get the variance of each bin from the average
        // variance = square of the abs difference between value and average
        for(int i = 0; i < pixelBins.size(); i++){
            varianceBins[i] = pow( pixelBins[i] - avgPixPerBin, 2 );
        }
        
        //now go through again and find the average of all the variances
        avgVariance = 0;
        for(int i = 0; i < varianceBins.size(); i++){
            avgVariance += varianceBins[i];
        }
        
        avgVariance /= (float)varianceBins.size();
        
        //standard deviation = sqrt of variance average
        stdDev = sqrt(avgVariance);
        
        
        if( stdDevBlackOutToggle ){
            
            if ( stdDev < stdDevThreshSlider ) {
                frameBlackOut = true;
                
                blackOutPix.allocate(camWidth, camHeight, 1);
                blackOutPix.setColor(ofColor(0));
            }
            
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
        if( !frameBlackOut ){
            contours.findContours(threshPix);
        } else {
            contours.findContours(blackOutPix);
        }
        
        
        
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
    ofDrawBitmapString("Cam Framerate: " + ofToString(camFrameRate), 10, 30);
    
    
    string title = "";
    
    
    if( currentView == 0 ){
        
        //----------STITCHING VIEW----------
        
        title = "Camera Stitching";
        
        ofPushMatrix();{
            
            ofTranslate(detectionDisplayPos);
            ofScale(compositeDisplayScale, compositeDisplayScale);
            
            ofImage img;
            
            ofSetColor(255);
            ofSetLineWidth(1);
            ofDrawBitmapString("Stitched Raw Camera View", 0, 0 - 5);
            img.setFromPixels(masterPix.getData(), masterWidth, masterHeight, OF_IMAGE_GRAYSCALE);
            img.draw(0, 0);
            
            ofNoFill();
            ofDrawRectangle(0, 0, masterWidth, masterHeight);
            
        }ofPopMatrix();
        
        
        
        
    } else if( currentView == 1 ){
        
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
            

            
            
            //ofImage wrapper we'll use to draw things to screen
            ofImage img;
            
            //----------slot 1----------
            ofSetColor(255);
            ofSetLineWidth(1);

            //cam 1
            ofDrawBitmapString("Cam 1 Raw", slot1.x, slot1.y - 5);
            img.setFromPixels(rawGrayPix1.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
            img.draw(slot1);
            
            ofNoFill();
            ofDrawRectangle(slot1, camWidth, camHeight);

            //cam 2
            ofDrawBitmapString("Cam 2 Raw", slot1.x, slot1.y - 5);
            img.setFromPixels(rawGrayPix2.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
            img.draw(slot1.x + camWidth + 10, slot1.y);
            
            ofNoFill();
            ofDrawRectangle(slot1.x + camWidth + 10, slot1.y, camWidth, camHeight);
            
            //cam 3
            ofDrawBitmapString("Cam 3 Raw", slot1.x, slot1.y - 5);
            img.setFromPixels(rawGrayPix3.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
            img.draw(slot1.x + camWidth*2 + 20, slot1.y);
            
            ofNoFill();
            ofDrawRectangle(slot1.x + camWidth*2 + 20, slot1.y, camWidth, camHeight);
            
            //draw black out X across foreground and threshold
            //if we're blacking out
            if( frameBlackOut ){
                
                ofPushStyle();
                ofSetColor(255, 0, 0);
                ofSetLineWidth(3);
                
                //Processed X
                ofDrawLine(slot2.x, slot2.y, slot2.x + camWidth, slot2.y + camHeight);
                ofDrawLine(slot2.x + camWidth, slot2.y, slot2.x, slot2.y + camHeight);
                
                ofPopStyle();
                
            }
            
            
            //----------slot 2----------
            
            //OSC status and info
            
            string oscData = "";
            
            oscData += "OSC Info\n";
            oscData += "------------------\n";
            oscData += "Destination IP:\n";
            oscData += oscIP + "\n";
            oscData += "Destination PORT:\n";
            oscData += ofToString(oscPort) + "\n";
            
            
            ofSetColor(255);
            ofDrawBitmapString(oscData, slot2.x, slot2.y + 15);
            
            string sentString = "OSC MESSAGE SENT...";
            float t = ofMap(ofGetElapsedTimef() - lastOSCSendTime, 0, 0.1, 255, 100, true);
            ofSetColor(255, 0, 0, t);
            ofDrawBitmapString(sentString, slot2.x, slot2.y + 120);
            
            
            //----------slot 3----------
            ofSetColor(255);
            ofDrawBitmapString("Stitched & Processed (+contrast/blur)", slot3.x, slot3.y - 5);
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
            ofTranslate(leftMargin + slot5.x*pipelineDisplayScale, topMargin + slot5.y*pipelineDisplayScale);
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

        
        
        //----------pixel distribution statistical info----------
        
        //put it in a pretty place
        ofPushStyle();
        ofPushMatrix();{
            
            ofVec2f graphOrigin(leftMargin, ofGetHeight() - 10);
            
            ofTranslate(graphOrigin);
            
            float maxYAxis = 130;
            float maxXAxis = 256;
            
            float horizontalMult = 3;
            
            auto it = std::max_element( pixelBins.begin(), pixelBins.end() );
            int maxBinHeight = *it;
            
            //draw axis lines
            ofSetColor(0, 128, 255);
            ofSetLineWidth(2);
            ofDrawLine(0, 0, 0, -maxYAxis);
            ofDrawLine(0, 0, maxXAxis * horizontalMult, 0);
            
            ofSetLineWidth(2);
            ofSetColor(255);
            for( int i = 0; i < pixelBins.size(); i++){
                
                float v  = ofMap(pixelBins[i], 0, maxBinHeight, 0, maxYAxis);
                
                //move it a few pixels to the right so it doesn draw on the axis line
                float x = ( i * horizontalMult ) + 2;
                ofDrawLine( x, 0, x, -v );
                
            }
            
            //draw the average line
            ofSetColor(255, 0, 0);
            ofSetLineWidth(2);
            ofDrawLine(pixelAverage*horizontalMult + 3, 0, pixelAverage*horizontalMult + 3, -maxYAxis);
            ofDrawBitmapString("Avg: " + ofToString(pixelAverage), pixelAverage*horizontalMult, -maxYAxis);
            
            string stats = "";
            
            stats += "PIXEL DISTRIBUTION\n";
            stats += "OF PROCESSED IMAGE\n";
            stats += "------------------\n";
            stats += "Average pixel value: " + ofToString(pixelAverage) + "\n";
            stats += "Average Variance: " + ofToString(avgVariance) + "\n";
            stats += "Standard Deviation: " + ofToString(stdDev) + "\n";
            
            ofSetColor(0, 180, 255);
            ofDrawBitmapString(stats, maxXAxis * horizontalMult + 20, -90);
            
            if( frameBlackOut ){
                string s = "FRAME BLACKOUT: \nPIXEL PROFILE TOO NOISY";
                ofSetColor(255, 0, 0);
                ofDrawBitmapString(s, maxXAxis * horizontalMult + 20, -130);
            }
            
            
        }ofPopStyle();
        ofPopMatrix();
        
        
    } else if( currentView == 2 ){
        
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
                    
                    ofPath slice = zones[activeZone].path;
                    slice.setFilled(true);
                    slice.setFillColor(ofColor(255, 150));
                    
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
        
        
        
        
        
    }
    
    ofSetColor(255);
    font.drawString(title, titlePos.x, titlePos.y);
    
    
    
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
    
    if( currentView == 2 ){
        
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
    gui.add(thresholdSlider.setup("Threshold", 0, 0, 255));
    gui.add(numErosionsSlider.setup("Number of erosions", 0, 0, 10));
    gui.add(numDilationsSlider.setup("Number of dilations", 0, 0, 10));
    gui.add(stdDevBlackOutToggle.setup("Std Dev Blackout", false));
    gui.add(stdDevThreshSlider.setup("Std Dev Thresh", 300, 0, 1000));
    
    gui.add(bgDiffLabel.setup("   BG SUBTRACTION", ""));
    gui.add(useBgDiff.setup("Use BG Diff", false));
    gui.add(learningTime.setup("Frames to learn BG", 100, 0, 300));
    gui.add(resetBGButton.setup("Reset Background"));
    
    gui.add(contoursLabel.setup("   CONTOUR FINDING", ""));
    gui.add(minBlobAreaSlider.setup("Min Blob Area", 0, 0, 1000));
    gui.add(maxBlobAreaSlider.setup("Max Blob Area", 1000, 0, 20000));
    gui.add(persistenceSlider.setup("Persistence", 15, 0, 100));
    gui.add(maxDistanceSlider.setup("Max Distance", 32, 0, 100));
    gui.add(drawContoursToggle.setup("Draw Contours", true));
    gui.add(showInfoToggle.setup("Info", false));
    
    gui.add(stitchingLabel.setup("   CAMERA STITCHING", ""));
    gui.add(trimMasterPixButton.setup("Trim pixels"));
    //make much bigger to accomodate
    //for different stitching layouts
    ofVec2f start(0, 0);
    ofVec2f end(camWidth*4, camHeight*4);
    
    gui.add(cam1Pos.setup("Cam 1 Position", ofVec2f(0, 0), start, end));
    gui.add(cam2Pos.setup("Cam 2 Position", ofVec2f(camWidth, 0), start, end));
    gui.add(cam3Pos.setup("Cam 3 Position", ofVec2f(camWidth*2, 0), start, end));
    
    
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
    
    
    
    //-----GUI 1 formatting-----
    gui.setHeaderBackgroundColor(ofColor(255));
    
    //color applies to gui title only
    gui.setTextColor(ofColor(0));
    
    imageAdjustLabel.setBackgroundColor(ofColor(255));
    bgDiffLabel.setBackgroundColor(ofColor(255));
    contoursLabel.setBackgroundColor(ofColor(255));
    detectionLabel.setBackgroundColor(ofColor(255));
    stitchingLabel.setBackgroundColor(ofColor(255));
    
    //this changes the color of all the labels
    contoursLabel.setDefaultTextColor(ofColor(0));
    
    
    //-----GUI 2 formatting-----
    gui2.setHeaderBackgroundColor(ofColor(255));
    
    //color applies to gui title only
    gui2.setTextColor(ofColor(0));
    
    zonePointsLabel.setBackgroundColor(ofColor(255));
    stitchingPointsLabel.setBackgroundColor(ofColor(255));
    
    //this changes the color of all the labels
    stitchingPointsLabel.setDefaultTextColor(ofColor(0));
    
    
}

void ofApp::loadSettings(){
    
    gui.loadFromFile(guiName + ".xml");
    gui2.loadFromFile(gui2Name + ".xml");
    
    gui2.setPosition(gui2Pos -> x, gui2Pos -> y);
    applyGuiValsToZones();
    
}

void ofApp::saveSettings(){
    
    gui.saveToFile(guiName + ".xml");
    gui2.saveToFile(gui2Name + ".xml");
    
}

void ofApp::drawGui(int x, int y){
    
    gui.setPosition(x, y);
    gui.draw();
    
    if( showSecondGui ){
        gui2.setPosition(gui2Pos -> x, gui2Pos -> y);
        gui2.draw();
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