#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    
    //gui setup
    setupGui();

    
    //setup pixel objects
    grayPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    processedPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    threshPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    backgroundPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    foregroundPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    
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
    leftMargin = 250;
    topmargin = 50;
    gutter = 30;
    
    slot1.set(leftMargin, topmargin);
    slot2.set(leftMargin + camWidth + gutter, topmargin);
    slot3.set(leftMargin + camWidth*2 + gutter*2, topmargin);
    slot4.set(leftMargin + camWidth*3 + gutter*3, topmargin);
    slot5.set(leftMargin + camWidth*3 + gutter*3, topmargin + camHeight + gutter);
    primarySlot.set(leftMargin, topmargin + camHeight + gutter*1.5);
    primarySlotScale = 3.0f;
    
    
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
    
    
    
    //Go through the zone points, and assign the point to the mouse
    //if it's being clicked
    adjustedMouse.set( ofGetMouseX() - primarySlot.x, ofGetMouseY() - primarySlot.y );
    
    //the camera view is scaled up, so scale the adjusted mouse down
    adjustedMouse /= primarySlotScale;

    //clamp to cam dimensions
    adjustedMouse.set( ofClamp( adjustedMouse.x, 0, camWidth),
                       ofClamp( adjustedMouse.y, 0, camHeight) );
    
    for( int i = 0; i < zones.size(); i++){

        zones[i].setClickedPoint( adjustedMouse.x, adjustedMouse.y );
        
    }
    
    
    
    
    thermal.checkForNewFrame();
    
	
    //new thermal cam frame?
	if(thermal.receivedNewFrame){

        cout << "[ofApp] Device Location: " << thermal.getDeviceLocation() << endl;
		
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
        grayImg = rawImg;

        //blur (need raw pixels from grayImg first to use ofxCv method)
        grayPix = grayImg.getPixels();
        grayPix.mirror(false, true);
        
        ofxCv::GaussianBlur(grayPix, processedPix, blurAmountSlider);
        
        
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
        
        for(int i = 0; i < processedPix.getWidth() * processedPix.getHeight(); i++){
            
            pixelAverage += processedPix[i];
            numSamples++;
            
            //add one to each bin depending on the pixel value
            pixelBins[ (int)processedPix[i] ] += 1;
            
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
    

    
    
    //OSC status and info
    
    string oscData = "";
    
    oscData += "OSC Info\n";
    oscData += "------------------\n";
    oscData += "Destination IP:\n";
    oscData += oscIP + "\n";
    oscData += "Destination PORT:\n";
    oscData += ofToString(oscPort) + "\n";
    

    ofSetColor(255);
    //draw under 4th slot
    ofDrawBitmapString(oscData, slot5.x, slot5.y + camHeight + gutter);
    
    string sentString = "OSC MESSAGE SENT...";
    float t = ofMap(ofGetElapsedTimef() - lastOSCSendTime, 0, 0.1, 255, 100, true);
    ofSetColor(255, 0, 0, t);
    ofDrawBitmapString(sentString, slot5.x, slot5.y + camHeight + gutter + 90);
    
    
    //ofImage wrapper we'll use to draw things to screen
    ofImage img;
    
    //----------slot 1----------
    ofSetColor(255);
    ofSetLineWidth(1);
    ofDrawBitmapString("Raw From Camera", slot1.x, slot1.y - 5);
    img.setFromPixels(grayPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    img.draw(slot1);
    
    ofNoFill();
    ofDrawRectangle(slot1, camWidth, camHeight);
    
    
    //----------slot 2----------
    ofSetColor(0, 128, 255);
    ofDrawBitmapString("Processed (+contrast/blur) -", slot2.x, slot2.y - 5);

    ofSetColor(255);
    img.setFromPixels(processedPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    img.draw(slot2);

    ofSetColor(0, 128, 255);
    ofNoFill();
    ofDrawRectangle(slot2, camWidth, camHeight);

    //----------slot 3----------
    ofSetColor(255);
    ofDrawBitmapString("Subtracted Background   =", slot3.x, slot3.y - 5);
    img.setFromPixels(backgroundPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    img.draw(slot3);
    
    ofNoFill();
    ofDrawRectangle(slot3, camWidth, camHeight);
    
    //----------slot 4----------
    ofDrawBitmapString("Foreground", slot4.x, slot4.y - 5);
    img.setFromPixels(foregroundPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    img.draw(slot4);
    
    ofNoFill();
    ofDrawRectangle(slot4, camWidth, camHeight);

    //----------slot 5----------
    ofDrawBitmapString("Thresholded", slot5.x, slot5.y - 5);
    img.setFromPixels(threshPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    img.draw(slot5);
    
    ofNoFill();
    ofDrawRectangle(slot5, camWidth, camHeight);
    
    
    //draw black out X across foreground and threshold
    //if we're blacking out
    if( frameBlackOut ){
        
        ofPushStyle();
        ofSetColor(255, 0, 0);
        ofSetLineWidth(3);
        
        //Foreground X
//        ofDrawLine(slot4.x, slot4.y, slot4.x + camWidth, slot4.y + camHeight);
//        ofDrawLine(slot4.x + camWidth, slot4.y, slot4.x, slot4.y + camHeight);

        //Thresholded X
        ofDrawLine(slot5.x, slot5.y, slot5.x + camWidth, slot5.y + camHeight);
        ofDrawLine(slot5.x + camWidth, slot5.y, slot5.x, slot5.y + camHeight);
        
        ofPopStyle();
        
    }
    
    
    
    //----------Primary Slot----------
    ofDrawBitmapString("Contours and Detection zones", primarySlot.x, primarySlot.y - 5);
    
    ofPushMatrix();{

        ofTranslate(primarySlot);
        ofScale(primarySlotScale, primarySlotScale);
        
        ofSetColor(0, 255);
        ofFill();
        ofDrawRectangle(0, 0, camWidth, camHeight);
        
        img.setFromPixels(threshPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
        ofSetColor(255, 100);
        img.draw(0, 0);
        
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
            
            
            
            //draw border
            ofSetColor(255);
            ofSetLineWidth(1);
            ofNoFill();
            ofDrawRectangle(0, 0, camWidth, camHeight);
        
        }ofPopStyle();
        
        
        //draw the pixel average text
        string s = "Average pixel value in Processed Img: " + ofToString(pixelAverage);
        ofSetColor(255);
        ofDrawBitmapString(s, 0, camHeight + 5);
        

        
        
    }ofPopMatrix();
    
    //----------pixel distribution statistical info----------

    //put it in a pretty place
    ofPushStyle();
    ofPushMatrix();{
        
        ofVec2f graphOrigin(primarySlot.x, ofGetHeight() - 10);
    
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
        ofDrawBitmapString(stats, maxXAxis * horizontalMult + 20, -100);
        
        if( frameBlackOut ){
            string s = "FRAME BLACKOUT: PIXEL PROFILE TOO NOISY";
            ofSetColor(255, 0, 0);
            ofDrawBitmapString(s, maxXAxis * horizontalMult + 20, -130);
        }
        
        
    }ofPopStyle();
    ofPopMatrix();
    
    //box with save/load feedback
    drawSaveLoadBox();

    
    drawGui(10, 40);
    
    
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

    //only check if the mouse is in the primary slot
    if( x > primarySlot.x && x < primarySlot.x + camWidth * primarySlotScale && y > primarySlot.y && y < primarySlot.y + camHeight * primarySlotScale){
        
        for( int i = 0; i < zones.size(); i++){
            
            //check for points, if one is found, stop looking
            //Also, adjust mouse to acount for frame position on screen
            if( zones[i].checkForClicks(adjustedMouse.x, adjustedMouse.y) ){
                break;
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
    
    

    gui.add(detectionLabel.setup("   DETECTION ZONES", ""));
    gui.add(showSecondGui.setup("Show Control Points", false));
    
    
    
    gui2Name = "controlPoints";
    gui2.setup(gui2Name, gui2Name + ".xml", 0, 0);
    gui2.add( gui2Pos.setup("Gui Pos", ofVec2f(200, 50), ofVec2f(0, 0), ofVec2f(ofGetWidth(), ofGetHeight())));
    
    ofVec2f start(0, 0);
    ofVec2f end(camWidth, camHeight);
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
    
    
    gui2.minimizeAll();
    
    

    
        


    
    
    gui.setHeaderBackgroundColor(ofColor(255));
    
    //color applies to gui title only
    gui.setTextColor(ofColor(0));
    
    imageAdjustLabel.setBackgroundColor(ofColor(255));
    bgDiffLabel.setBackgroundColor(ofColor(255));
    contoursLabel.setBackgroundColor(ofColor(255));
    detectionLabel.setBackgroundColor(ofColor(255));
    
    //this changes the color of all the labels
    contoursLabel.setDefaultTextColor(ofColor(0));

    
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
    
    ofVec2f settingsDialogPos( ofGetWidth() - 140 , ofGetHeight() - 30);
    ofFill();
    
    if(ofGetElapsedTimef() - lastSaveTime < 1.0f){
        ofSetColor(0, 180, 0);
        ofDrawRectangle(settingsDialogPos.x, settingsDialogPos.y, 125, 25);
        
        ofSetColor(255);
        ofDrawBitmapString("Settings Saved", settingsDialogPos.x + 5, settingsDialogPos.y + 18);
    }
    
    if(ofGetElapsedTimef() - lastLoadTime < 1.0f){
        ofSetColor(0, 128, 255);
        ofDrawRectangle(settingsDialogPos.x, settingsDialogPos.y, 130, 25);
        
        ofSetColor(255);
        ofDrawBitmapString("Settings loaded", settingsDialogPos.x + 5, settingsDialogPos.y + 18);
    }
    
}