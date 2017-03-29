#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    
    //gui setup
    setupGui();
    loadSettings();
    
    //setup pixel objects
    rawImg.allocate(camWidth, camHeight);
    grayImg.allocate(camWidth, camHeight);

    processedPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    threshPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    backgroundPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
	
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
    
    //get gui values and update zone paths
    applyGuiValsToZones();
    
    
    //content layout
    leftMargin = 270;
    topmargin = 50;
    gutter = 30;
    
    slot1.set(leftMargin, topmargin);
    slot2.set(leftMargin + camWidth + gutter, topmargin);
    slot3.set(leftMargin + camWidth*2 + gutter*2, topmargin);
    slot4.set(leftMargin + camWidth*3 + gutter*3, topmargin);
    primarySlot.set(leftMargin, topmargin + camHeight + gutter*1.5);
    primarySlotScale = 3.0f;
    
    
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
    
    //set detection regions according to gui values
//    applyGuiValsToZones();
    
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
		
        //log the frame times (and the second to last one to
        //average it out and smooth the value)
        float thisFrameRate = 1.0/( (ofGetElapsedTimef() - lastFrameTime) );
        
        //average this framerate with the last one to smooth out numbers
        //and get a better reading.
        camFrameRate = (thisFrameRate + lastFrameRate)/2;
        lastFrameRate = thisFrameRate;

        lastFrameTime = ofGetElapsedTimef();
        
        
        //get pix from cam
        rawImg.setFromPixels(thermal.getPixels(), camWidth, camHeight);
        
        //convert RGBA camera data to single grayscale ofPixels
        grayImg = rawImg;

        //blur (need raw pixels from grayImg first to use ofxCv method)
        ofPixels gray;
        gray = grayImg.getPixels();
        
        ofxCv::GaussianBlur(gray, processedPix, blurAmountSlider);
        
        
        //Adjust contrast
        for(int i = 0; i < processedPix.getWidth() * processedPix.getHeight(); i++){
            
            //normalized pixel value
            float normPixVal = processedPix[i]/255.0f;
            //
            processedPix[i] = ofClamp( 255 * pow((normPixVal + contrastPhaseSlider), contrastExpSlider), 0, 255);
        }
        
        
        
        //if we're using the blackout method, find the average pixel
        //and if greater than threshold, feed a blacked out ofPixels instance
        //into the contour finder, but keep the original threshPix untouched
        //for drawing to screen
        frameBlackOut = false;
        
        //needs to be in current scope, but no need to allocate/fill
        //unless we're using the blackout method below
        ofPixels blackOutPix;
        
        processedPixelAvg = 0;
        
        for(int i = 0; i < processedPix.getWidth() * processedPix.getHeight(); i++){
            processedPixelAvg += processedPix[i];
        }
        
        processedPixelAvg /= processedPix.getWidth() * processedPix.getHeight();

        if( averagePixelsToggle ){
            
            if ( processedPixelAvg > blackOutThreshSlider ) {
                frameBlackOut = true;
                
                blackOutPix.allocate(camWidth, camHeight, 1);
                blackOutPix.setColor(ofColor(0));
            }
            
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
            
            //get the foreground to draw to screen
            ofxCv::toOf( background.getBackground(), backgroundPix );
            
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
    
    drawGui(10, 40);
    
    
    //OSC status and info
    
    string oscData = "";
    
    oscData += "OSC Info\n";
    oscData += "------------------\n";
    oscData += "Destination IP:\n";
    oscData += oscIP + "\n";
    oscData += "Destination PORT:\n";
    oscData += ofToString(oscPort) + "\n";
    
    ofSetColor(0, 180, 255);
    //draw under 4th slot
    ofDrawBitmapString(oscData, slot4.x, primarySlot.y);
    
    string sentString = "OSC MESSAGE SENT...";
    float t = ofMap(ofGetElapsedTimef() - lastOSCSendTime, 0, 0.1, 255, 100, true);
    ofSetColor(255, 0, 0, t);
    ofDrawBitmapString(sentString, slot4.x, primarySlot.y + 90);
    
    
    
    //----------slot 1----------
    ofSetColor(255);
    ofSetLineWidth(1);
    ofDrawBitmapString("Raw From Camera", slot1.x, slot1.y - 5);
    
    rawImg.draw(slot1);
    
    ofNoFill();
    ofDrawRectangle(slot1, camWidth, camHeight);
    
    
    //----------slot 2----------
    ofDrawBitmapString("Processed (+contrast/blur) -", slot2.x, slot2.y - 5);
    ofImage img;
    img.setFromPixels(processedPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    img.draw(slot2);

    ofNoFill();
    ofDrawRectangle(slot2, camWidth, camHeight);

    //----------slot 3----------
    ofDrawBitmapString("Subtracted Background   =", slot3.x, slot3.y - 5);
    img.setFromPixels(backgroundPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    img.draw(slot3);
    
    ofNoFill();
    ofDrawRectangle(slot3, camWidth, camHeight);
    
    //----------slot 4----------
    ofDrawBitmapString("Threshold (+eros./dil.)", slot4.x, slot4.y - 5);
    img.setFromPixels(threshPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    img.draw(slot4);
    
    ofNoFill();
    ofDrawRectangle(slot4, camWidth, camHeight);
    
    
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
        
        //draw black out X across image
        if( frameBlackOut ){
            
            ofPushStyle();
            ofSetColor(255, 0, 0);
            ofSetLineWidth(3);
            ofDrawLine(0, 0, camWidth, camHeight);
            ofDrawLine(0, camHeight, camWidth, 0);
            
            ofPopStyle();
            
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
        string s = "Average pixel value in Processed Img: " + ofToString(processedPixelAvg);
        ofSetColor(255);
        ofDrawBitmapString(s, 0, camHeight + 5);
        
        if( frameBlackOut ){
            string s = "FRAME BLACKOUT: Avg pixel brightness exceeds thresh";
            ofSetColor(255, 0, 0);
            ofDrawBitmapString(s, 0, camHeight + 10);
        }
        
        
    }ofPopMatrix();
    
    
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

    for( int i = 0; i < zones.size(); i++){
        
        //check for points, if one is found, stop looking
        //Also, adjust mouse to acount for frame position on screen
        if( zones[i].checkForClicks(adjustedMouse.x, adjustedMouse.y) ){
            break;
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
    
    //param 1 = gui title, param 2 = filename, then 3&4 = startposition
    gui.setup(guiName, guiName + ".xml", 0, 0);
    
    gui.add(imageAdjustLabel.setup("   IMAGE ADJUSTMENT", ""));
    gui.add(blurAmountSlider.setup("Blur", 1, 0, 40));
    gui.add(contrastExpSlider.setup("Contrast Exponent", 1.0, 1.0, 8.0));
    gui.add(contrastPhaseSlider.setup("Contrast Phase", 0.0, 0.0, 0.4));
    gui.add(thresholdSlider.setup("Threshold", 0, 0, 255));
    gui.add(numErosionsSlider.setup("Number of erosions", 0, 0, 10));
    gui.add(numDilationsSlider.setup("Number of dilations", 0, 0, 10));
    gui.add(averagePixelsToggle.setup("Pixel Blackout", false));
    
    gui.add(blackOutThreshSlider.setup("Blackout Thresh", 200, 0, 255));
    
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
    
    
    ofVec2f start(0, 0);
    ofVec2f end(camWidth, camHeight);

    gui.add(detectionLabel.setup("   DETECTION ZONES", ""));
    gui.add( dangerPt0.setup("Danger Z. Pt 0", start, start, end));
    gui.add( dangerPt1.setup("Danger Z. Pt 1", start, start, end));
    gui.add( dangerPt2.setup("Danger Z. Pt 2", start, start, end));
    gui.add( dangerPt3.setup("Danger Z. Pt 3", start, start, end));
    gui.add(active1Pt0.setup("Active Z-1 Pt 0", start, start, end));
    gui.add(active1Pt1.setup("Active Z-1 Pt 1", start, start, end));
    gui.add(active1Pt2.setup("Active Z-1 Pt 2", start, start, end));
    gui.add(active1Pt3.setup("Active Z-1 Pt 3", start, start, end));
    gui.add(active2Pt0.setup("Active Z-2 Pt 0", start, start, end));
    gui.add(active2Pt1.setup("Active Z-2 Pt 1", start, start, end));
    gui.add(active2Pt2.setup("Active Z-2 Pt 2", start, start, end));
    gui.add(active2Pt3.setup("Active Z-2 Pt 3", start, start, end));
    gui.add(active3Pt0.setup("Active Z-3 Pt 0", start, start, end));
    gui.add(active3Pt1.setup("Active Z-3 Pt 1", start, start, end));
    gui.add(active3Pt2.setup("Active Z-3 Pt 2", start, start, end));
    gui.add(active3Pt3.setup("Active Z-3 Pt 3", start, start, end));
    
    
    gui.minimizeAll();
    
    
//        gui.add(maskingLabel.setup("   MASKING", ""));
//        gui.add(useMask.setup("Use Mask", true));
//        gui.add(drawOrErase.setup("Draw or Erase", true));
//        gui.add(clearMask.setup("Clear Mask"));
//        gui.add(saveMask.setup("Save Mask"));
//        gui.add(loadMask.setup("Load Mask"));
    
        


    
    
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
    
}

void ofApp::saveSettings(){
    
    gui.saveToFile(guiName + ".xml");
    
}

void ofApp::drawGui(int x, int y){
    
    gui.setPosition(x, y);
    gui.draw();
    
}




void ofApp::drawSaveLoadBox(){
    
    ofVec2f settingsDialogPos( ofGetWidth() - 170 , ofGetHeight() - 50);
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