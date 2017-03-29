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

    cout << "Setting OSC destination IP: " << oscIP << " on Port: " << oscPort << endl;
    
    osc.setup(oscIP, oscPort);
    
    //Time of last osc messages sent
    //Pre-date them so they don't draw on start up
    lastActiveTriggerTime = -10;
    lastDangerTriggerTime = -10;
    
    
    bInZone.assign(4, false);
    
    
    lastFrameTime = 0;
    lastFrameRate = 0;
    camFrameRate = 0;
    
    frameBlackOut = false;
    
}

//--------------------------------------------------------------
void ofApp::update(){
    
    //set detection regions according to gui values
    
    detectionZones.clear();
    
    detectionZones = {
        
        ofRectangle((ofPoint)dangerRegionStart, (ofPoint)dangerRegionEnd),
        ofRectangle((ofPoint)activeRegion3Start, (ofPoint)activeRegion3End),
        ofRectangle((ofPoint)activeRegion2Start, (ofPoint)activeRegion2End),
        ofRectangle((ofPoint)activeRegion1Start, (ofPoint)activeRegion1End)
        
    };
    
    
    
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
        int zoneTriggered = -1;
        
        //for each ZONE...
        for(int i = 0; i < detectionZones.size(); i++){
            
            //for each CONTOUR...
            for(int j = 0; j < contours.size(); j++){
                
                ofPolyline points = contours.getPolyline(j).getVertices();
                
                //for each POINT...
                for(int k = 0; k < points.size(); k++){
                
                    if( detectionZones[i].inside(points[k]) ){
                        foundObject = true;
                        zoneTriggered = i;
                        
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

        //set detection booleans to false
        bInZone.clear();
        bInZone.assign(4, false);
        
        //if we found something, send the OSC message
        if( foundObject ){
            
            ofxOscMessage m;
            
            m.setAddress("/Detected");
            m.addIntArg(zoneTriggered);
            
            osc.sendMessage(m);
            
            //and set the detection booleans accordingly for visualization purposes
            bInZone[zoneTriggered] = true;
            

            
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
    
    
    //content layout
    float leftMargin = 270;
    float topmargin = 50;
    float gutter = 30;
    
    ofVec2f slot1(leftMargin, topmargin);
    ofVec2f slot2(leftMargin + camWidth + gutter, topmargin);
    ofVec2f slot3(leftMargin + camWidth*2 + gutter*2, topmargin);
    ofVec2f slot4(leftMargin + camWidth*3 + gutter*3, topmargin);
    
    ofVec2f primarySlot(leftMargin, topmargin + camHeight + gutter*1.5);
    
    
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
        ofScale(3.0, 3.0);
        
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

            ofNoFill();
            ofSetLineWidth(4);
            
            int alpha = 150;
            
            for(int i = detectionZones.size() - 1; i >= 0 ; i--){
                
                if( bInZone[i] ){
                    
                    //draw a transparent rect also
                    ofFill();
                    ofSetColor(255, 150);
                    ofDrawRectangle(detectionZones[i]);

                }
                
                ofNoFill();
                
                if( i == 0 ){
                    ofSetColor(255, 0, 0, alpha);
                } else if( i == 1 ){
                    ofSetColor(255, 100, 0, alpha);
                } else if( i == 2 ){
                    ofSetColor(255, 200, 0, alpha);
                } else{
                    ofSetColor(0, 255, 0, alpha);
                }
                
                ofDrawRectangle(detectionZones[i]);
                
                
                
                //Draw the slice of the area that we're in: the outer detection zone
                //minus the inner detection zone
                
            }
            
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

    cout << mouseX << ", " << mouseY << endl;
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

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
    gui.add(activeRegion1Start.setup("Active1 Top Left", start, start, end));
    gui.add(activeRegion1End.setup("Active1 Bot. Right", end, start, end));
    gui.add(activeRegion2Start.setup("Active2 Top Left", start, start, end));
    gui.add(activeRegion2End.setup("Active2 Bot. Right", end, start, end));
    gui.add(activeRegion3Start.setup("Active3 Top Left", start, start, end));
    gui.add(activeRegion3End.setup("Activ3 Bot. Right", end, start, end));
    gui.add(dangerRegionStart.setup("Danger Top Left", start, start, end));
    gui.add(dangerRegionEnd.setup("Danger Bot. Right", end, start, end));
    
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