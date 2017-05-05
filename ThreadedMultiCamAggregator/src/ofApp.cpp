#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    
    ofSetFrameRate(60);
    ofSetVerticalSync(false);
    ofSetLogLevel(OF_LOG_VERBOSE);
    
    //gui setup
    setupGui();
    
    //setup pixel objects

    
    processedPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    threshPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    backgroundPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    foregroundPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    
    blackFrame.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    blackFrame.setColor(0);

    //same as above but rotated 90 degrees
    blackFrameRot90.allocate(camHeight, camWidth, OF_IMAGE_GRAYSCALE);
    blackFrameRot90.setColor(0);
    
    //masterPix object will get immediately re-allocated when things are repositioned,
    //but allocate it much larger to start just to be safe
    masterPix.allocate(camWidth*3, camWidth*3, OF_IMAGE_GRAYSCALE);
    
    //keep track of the dimensions of the running background
    backgroundWidth = 0;
    backgroundHeight = 0;
    

    
    
    
    
    //time of the last save/load event
    //start them out earlier so they dont draw on startup
    lastLoadTime = -10.0f;
    lastSaveTime = -10.0f;
    
    
    
    //----------OSC setup----------
    
    //get the IP/Port from file
    ofBuffer buffer = ofBufferFromFile("osc.txt");
    
//    oscIP = "";
//    oscPort = 0;
//    
//    oscIP = "169.254.168.115";
//    oscPort = 12345;
    
    
    
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
    lastStatusSendTime = 0;
    lastStatusSendTime = 0;
    
    
    
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
                
//            case 3:
//                zones[i].setGuiRefs(&active3Pt0, &active3Pt1, &active3Pt2, &active3Pt3);
//                break;
                
            default:
                break;
        }
        
    }
    
    

    
    //-----------------------------------------
    //----------content layout and UI----------
    //-----------------------------------------
    
    //0 = "Headless" view
    //1 = All Cameras
    //2 = Cams 0-1
    //3 = Cams 2-3
    //4 = Cams 4-5
    //5 = stitching mode view
    //6 = Masking view
    //7 = Pipeline
    //8 = zones view
    //9 = Camera Addressing
    currentView = HEADLESS;
    
    appStatus = 0;
    
    titleFont.load("fonts/Aller_Rg.ttf", 40);
    smallerFont.load("fonts/Aller_Rg.ttf", 16);
    
    //arrange content to start with, will be changed later
    //based on stitched view dimensions
    leftMargin = 250;
    topMargin = 75;
    gutter = 30;
    
    titlePos.set(leftMargin, titleFont.stringHeight("Ag"));
    
    
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
    
    maskScreenPos.set(leftMargin, topMargin + 10);
    
    
    //load up the gui settings
    loadSettings();
    
    
    //get gui values and update zone paths
    applyGuiValsToZones();
    

    //-----------------------------------------
    //-----------------Cameras-----------------
    //-----------------------------------------
    
    //Setup thermal camera framework
    thermal.setup();
    
    //add a listener to the thermal client's new frame event
    ofAddListener( thermal.newFrameEvt, this, &ofApp::addNewFrameToQueue );
    

    
    listenForNewAddresses = false;
    
    //give the address panel a reference to the
    //address vector
    addressPanel.setup( &addresses );

    
    
    
    
    //setup the individual feed objects
    feeds.resize(TOTAL_NUM_CAMS);
    for( int i = 0; i < feeds.size(); i++){
        
        //cam number, USB ID, width and height
        feeds[i].setup( i, addresses[i], camWidth, camHeight );
        
        //set refs to GUI vals
        feeds[i].contrastExp = &contrastExpSlider;
        feeds[i].contrastPhase = &contrastPhaseSlider;
        feeds[i].stdDevThresh = &stdDevThreshSlider;
        feeds[i].stdDevToggle = &stdDevBlackOutToggle;
    }
    
    

    maskToolSize = 10;
    maskCol.set(255, 200, 0);
    
    
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
                
//            case 3:
//                zones[i].setPoints(active3Pt0, active3Pt1, active3Pt2, active3Pt3);
//                break;
                
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
    maskGuiPos = maskingGui.getPosition();
    
    float contentAreaWidth = ofGetWidth() - leftMargin - 20; //make room for margin on the right
    float contentAreaHeight = ofGetHeight() - topMargin;
    
    //scale for fitting one large composite view on screen
    compositeDisplayScale = contentAreaWidth/(float)masterWidth;
    
    //scale for fitting all the CV pipeline images on screen
    //choose minimum between width and height scale
    pipelineDisplayScale = std::min( contentAreaWidth/(float)( masterWidth * 2 + gutter ), contentAreaHeight/(float)( masterHeight*2 + gutter*1 ) );

    
    
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
    
    //adjusted mouse position for drawing in the mask
    maskMousePos.set( ofGetMouseX() - maskScreenPos.x, ofGetMouseY() - maskScreenPos.y );
    
    
    //--------------------MASK MANAGEMENT--------------------
    if(clearMask){
        maskPix.setColor(0);
    }
    
    if(saveMask){
        maskImg.setFromPixels(maskPix);
        maskImg.save(maskFileName);
    }
    
    if(loadMask){
        maskImg.load(maskFileName);
        maskPix = maskImg.getPixels();
    }
    
    
    //drawing inside the mask
    //if we're inside the mask and we're in masking view
    if( currentView == MASKING && maskMousePos.x > 0 && maskMousePos.x < maskPix.getWidth() && maskMousePos.y > 0 && maskMousePos.y < maskPix.getHeight() ){
        
        bMouseInsideMask = true;
        
        if( ofGetMousePressed() ){
            
            //go through all the pixels inside the cursor
            //and set the pixel value to white.
            //Going through the cursor pixels is faster
            //than going through all the pixels of the mask pixel object
            for(int i = 0; i < maskToolSize * maskToolSize; i++){
                
                //XY value from the for loop iterator
                int x = i % maskToolSize;
                int y = (i - x)/maskToolSize;
                
                //then adjust to the mouse position within the pixel object
                //(also minus half because the cursor is centered at mouse)
                x += maskMousePos.x - maskToolSize/2;
                y += maskMousePos.y - maskToolSize/2;
                
                //only set pixels of the mask for cursors pixels that
                //are actually inside (near-border cases)
                if(x >= 0 && y >= 0 && x <= maskPix.getWidth() && y <= maskPix.getHeight()){
                    
                    int pixel = y * maskPix.getWidth() + x;
                    
                    int value = (drawOrErase ? 255 : 0);
                    
                    maskPix.setColor(pixel, ofColor(value));
                }
                
            }

            
            
            
        }
        
        
    } else {
        
        bMouseInsideMask = false;
    }

    
    if( currentView == MASKING && bMouseInsideMask ){
        //hide the cursor if we're drawing in the mask
        cursorShowing = false;
    } else {
        cursorShowing = true;
    }
    
    
    //we need to do a weird debounce on the show/hide cursor
    //since there seems to be a bug where a build up occurs if
    //ofShowCursor() is called every frame.

    //if this frame is different from the last one, do something
    if( lastFrameCursorShowing != cursorShowing ){
        if( cursorShowing ){
            ofShowCursor();
        } else {
            ofHideCursor();
        }
    }
    
    //store the value for the next frame
    lastFrameCursorShowing = cursorShowing;
    
    
    
    
    
    //-----Camera Address Management-----
    if( resetCamAddresses ){
        
        addresses.clear();
        addresses.assign(TOTAL_NUM_CAMS, 0);
        
        //reset the ids from the feeds too
        for(int i = 0; i < feeds.size(); i++){
            feeds[i].camID = 0;
            
            //also clear all the images
            feeds[i].resetAllPixels();
        }
        
        listenForNewAddresses = true;
        
    }
    
    
    addressPanel.update();
    
    //reset the feed IDs to whatever is in the address vector
    for(int i = 0; i < feeds.size(); i++){
        feeds[i].camID = addresses[i];
    }
    
    
    //This method will check for a new frame then trigger
    //an event that will notify the "addFrameToQueue()"
    //method and add the data to the queue
    thermal.checkForNewFrame();
    
    
    //new thermal cam frame?
    if( frameQueue.size() > 0 ){
        
//        cout << "New Frames: " << frameQueue.size() << endl;
        
        //--------------------NEW FRAME -> FEED ASSIGNMENT--------------------
        
        //get all te frames from the queue
        //and insert them into the feeds
        do{
            
            //find which camera the frame is from
            int thisCamId = (*frameQueue.begin()).ID;
            
//            cout << "New frame from: " << thisCamId << endl;
            
            if( listenForNewAddresses ){
                
                //see if this id exists in the address vector
                //if not, replace it with the first available slot
                int availableSlot = -1;
                bool existingID = false;
                
                for(int i = 0; i < addresses.size(); i++){
                    
                    //find first available slot in the address vector
                    //in case we need to add a new address
                    if( addresses[i] == 0 && availableSlot == -1 ){
                        availableSlot = i;
                    }
                    
                    //check new address against vector
                    if( addresses[i] == thisCamId ){
                        
                        //it maches, nothing to do, just move on
                        existingID = true;
                        break;
                    }
                    
                }
                
                
                if( !existingID ){
                    
                    addresses[availableSlot] = thisCamId;
                    
                    //set the feed to have this address too
                    feeds[availableSlot].camID = thisCamId;
                    
                }
                
                //if we've gotten this far and there are no more available
                //slots then we've filled them all, no need to listem to more
//                if( availableSlot == -1 ){
//                    listenForNewAddresses = false;
//                }
            
            
            }
            
            
            
            
            //get pix from cam
            ofxCvColorImage rawImg;
            rawImg.allocate(camWidth, camHeight);
            rawImg.setFromPixels( (*frameQueue.begin()).pix.getData() , camWidth, camHeight);
            
            //convert RGBA camera data to single grayscale ofPixels
            ofxCvGrayscaleImage grayImg;
            grayImg.allocate(camWidth, camHeight);
            grayImg = rawImg;
            
            
            ofPixels raw;
            raw.setFromPixels(grayImg.getPixels().getData(), camWidth, camHeight, 1);
            
            //blur it
            grayImg.blurGaussian(blurAmountSlider);
            
            //create a pixel object
            ofPixels gray;
            gray.setFromPixels(grayImg.getPixels().getData(), camWidth, camHeight, 1);
            
            //put the camera frame into the appropriate feed object
            //Also do any flipping and adjust contrast since it's new data
            int whichCam = -1;
            
            //find which cam it belongs to and
            for(int i = 0; i < feeds.size(); i++){
                
                //Check current frame ID against feed ID
                if( feeds[i].camID == thisCamId ){

                    if( camMirrorToggles[i] ){
                        raw.mirror(false, true);
                        gray.mirror(false, true);
                    }

                    whichCam = i;
                    
                    //send the raw and gray frames into the feed object
                    feeds[i].newFrame( raw, gray );
                    
                    break;
                }
            }
            
            //debug
            if( whichCam == -1 ){
//                cout << "ID not recognized: " << thisCamId <<endl;
            }

        
            //Now that we've retrieved the data from the queue, get rid of the oldest one.
            //App framerate is much faster than even 3 incoming cameras so no need
            //to process multiple frames in one pass
            frameQueue.pop_front();

            
        } while ( frameQueue.size() );

        
        
        
        
        
        //--------------------COMPOSITE IMAGE CONSTRUCTION--------------------
        

        
        //get the furthest Right and down parts of the aggregated image
        //so the masterPix object can be resized appropriately
        int furthestRight = 0;
        int furthestDown = 0;
        int furthestLeft = 10000;
        int furthestUp = 10000;
        

        for(int i = 0; i < TOTAL_NUM_CAMS; i++){
            
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
            for( int i = 0; i < TOTAL_NUM_CAMS; i++){
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

            
            //now store the new dims as old ones
            oldMasterHeight = masterHeight;
            oldMasterWidth = masterWidth;
            
            cout << "Re-allocating pixel objects" << endl;
        
        }

        
        //all feeds are blended into masterPix object so we need to
        //clear it every frame to avoid pixel build up
        masterPix.setColor(ofColor(0));
        
        
        //Now paste the new frame into the masterPix object
        //with any rotations specified
        for (int i = 0; i < TOTAL_NUM_CAMS; i++){
            
            ofPixels feedOutput = feeds[i].getOutputPix();
            
            if( camRotations[i] != 0 ){
                feedOutput.rotate90( camRotations[i] );
            }
            
            feedOutput.blendInto(masterPix, camPositions[i] -> x, camPositions[i] -> y);
            
        }
        
        
        //masterPix will hold the raw composite pixels
        //We'll subtract the mask from it and store it in processedPix
        if(useMask){
            
            
            //check the dimensions of the maskPix vs the
            //processedPix object before we do the subtraction
            if( maskPix.getWidth() != processedPix.getWidth() || maskPix.getHeight() != maskPix.getHeight() ){
                
                cout << "Re-allocating mask to match processedPix dimensions" << endl;
                cout << "Old mask dims: " << maskPix.getWidth() << ", " << maskPix.getHeight() << endl;
                
                //if there's a difference, make a copy with the proper dims,
                //paste the mask into it then save it into the mask
                ofPixels newMask;
                newMask.allocate(processedPix.getWidth(), processedPix.getHeight(), OF_IMAGE_GRAYSCALE);

                cout << "New mask dims: " << newMask.getWidth() << ", " << newMask.getHeight() << endl;
                
                
                newMask.setColor(0);
                
                //pasteInto() will not work if destination is smaller than pix being pasted
                //Method should crop on its own, but it doesn't so we'll do it manually
                if ( maskPix.pasteInto(newMask, 0, 0) ){
                    
                    cout << "Paste successful" << endl;
                    
                } else {
                    
                    cout << "Paste not successful, using cropTo() instead." << endl;
                    maskPix.cropTo(newMask, 0, 0, newMask.getWidth(), newMask.getHeight());
                    
                    
                }
                
                maskPix = newMask;
                
                
                //save the mask
                maskImg.setFromPixels(maskPix);
                maskImg.save(maskFileName);
                
                
            }
            
            // master - mask = processed
            ofxCv::subtract(masterPix, maskPix, processedPix);

        
        } else {
            
            //if we're not using the mask, just put masterPix directly into processedPix
            processedPix = masterPix;
            
            
        }
        
        
        
        //threshold if we're not using the running background
        //otherwise, ofxCv::RunningBackground already returns a thresholded image
        if( !useBgDiff ){
            //set flag to true in case we switch back to using BG diff again
            bNeedBGReset = true;
            
            //fill the back/foreground objects with something
            //to clear buffer garbage being drawn to screen
            backgroundPix.setColor(70);
            
            //without BG subtraction, foreground is essentially just the processed pix
            foregroundPix = processedPix;
            
            ofxCv::threshold(processedPix, threshPix, thresholdSlider);
            
            
        } else {
            
            //if the dimensions of the incoming master width and height
            //the background will need to be reset or it will crash
            if( backgroundWidth != masterWidth || backgroundHeight != masterHeight ){
                bNeedBGReset = true;
                backgroundWidth = masterWidth;
                backgroundHeight = masterHeight;
            }
            
            if(bNeedBGReset || resetBGButton){
                
                background.reset();
                bNeedBGReset = false;
                
            }
            
            background.setDifferenceMode(ofxCv::RunningBackground::BRIGHTER);
            background.setLearningTime(learningTime);
            background.setThresholdValue(thresholdSlider);
            
            background.update(processedPix, threshPix);
            
            //get the foreground/background to draw to screen
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
        
        
        
        
        //if we found something and OSC is switched on, send the message
        if( foundObject && sendOSCToggle ){
            
            //only send at the desired rate && wait after startup
            if( ofGetElapsedTimef() - lastZoneSendTime > maxOSCSendRate && ofGetElapsedTimef() > waitBeforeOSCSlider ){
                
                ofxOscMessage zone;
                
                zone.setAddress("/detected");
                zone.addIntArg( activeZone );
                zone.addIntArg( contours.size() );
                
                osc.sendMessage(zone);
                
                
                lastZoneSendTime = ofGetElapsedTimef();
            
            }
            
        }
        
        
        
        
        
    } else {
        
//        cout << "No new frame data this loop. Doing nothing." << endl;
        
    }  //frame queue check
    

    //if it's been long enough after start and long enough since last send time
    if( ofGetElapsedTimef() - lastStatusSendTime > statusSendRate ){

        //if there are ANY cameras that havent sent a
        //frame in over 5 seconds status is NOT OK

        //0 = warming up, 1 = OK, 2 = NOT OK
        if( ofGetElapsedTimef() < waitBeforeOSCSlider ){
            //warming up
            appStatus = 0;
        } else {
            
            //check all the cameras and the last time we got a frame from them.
            //active cameras are listed in the address vector as non zero, so
            //just check the cameras with a non-zero address for the last time they
            //were updated
            float longestTimeSinceUpdate = 0;
            
            for( int i = 0; i < addresses.size(); i++){
                
                if( addresses[i] > 0 ){
                    
//                    cout << "Feed[" << i << "] last frame time: " << feeds[i].lastFrameTime << "     ";
                    float sinceUpdate = ofGetElapsedTimef() - feeds[i].lastFrameTime;
                    
                    if( sinceUpdate > longestTimeSinceUpdate ){
                        longestTimeSinceUpdate = sinceUpdate;
                    }
                }
            }
            
//            cout << endl;
            
            if( longestTimeSinceUpdate > sysNotOKSlider ){
                //NOT OK (been more than 3 seconds since frame from camera
                appStatus = 2;
            } else {
                //everything OK
                appStatus = 1;
            }
            
        }
            
            
        
        ofxOscMessage statusMessage;
        statusMessage.setAddress("/status");
        statusMessage.addInt32Arg(appStatus);
        
        osc.sendMessage(statusMessage);
        
        lastStatusSendTime = ofGetElapsedTimef();
        
    }
    
    
    
    
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    if( appStatus == 2 ){
        //App NOT OK
        ofBackground(180,0,0);
    } else {
        //Everything's fine
        ofBackground(50);
    }
    
    


    
    
    string title = "";
    

    if( currentView == HEADLESS ){
        title = "\"Headless\" Mode";
        
        //screen with some info, but mostly blank to
        //save CPU cycles if we don't need to see anything
        
        //cam repositioning warning
        ofSetColor(0, 255, 0);
        string s = "Save a little MacMini CPU by not drawing anything to screen unless needed";
        smallerFont.drawString(s, leftMargin, topMargin + smallerFont.stringHeight("Ag")*2);
        
        
        //OSC status and info
        
        string oscData = "";
        
        oscData += "OSC Info\n";
        oscData += "------------------\n";
        oscData += "Destination IP:\n";
        oscData += oscIP + "\n";
        oscData += "Destination PORT:\n";
        oscData += ofToString(oscPort) + "\n";
        oscData += "\n";
        oscData += "Message Structure\n";
        oscData += "------------------\n";
        oscData += "Detection Zone Message:\n";
        oscData += "    Address: /detected\n";
        oscData += "    int32 Arg: active zone #\n";
        oscData += "        -1 = no one in any zone\n";
        oscData += "        0 = blob in danger zone\n";
        oscData += "        1-3 = blob in this interaction zone\n";
        oscData += "\n";
        oscData += "Status Message:\n";
        oscData += "    Address: /status\n";
        oscData += "    int32 Arg: status #\n";
        oscData += "        0 = warming up\n";
        oscData += "        1 = OK\n";
        oscData += "        2 = NOT OK\n";
        oscData += "        (NOT OK = more than 3 seconds since a camera has updated)\n";
        
        ofSetColor(255);
        ofDrawBitmapString(oscData, leftMargin, topMargin + 100);

        
        string sentString;
        float t = ofMap(ofGetElapsedTimef() - lastStatusSendTime, 0, 0.1, 255, 100, true);
        
        if( sendOSCToggle ){
            ofSetColor(0, 255, 0, t);
            sentString = "OSC MESSAGE SENT...";
        } else {
            ofSetColor(255, 0, 0);
            sentString = "OSC NOT SENDING";
        }
        
        ofVec2f sentPos(leftMargin + 170, topMargin + 100);
        
        
        ofPushStyle();
        ofNoFill();
        ofDrawRectangle(sentPos.x - 5, sentPos.y - 15, 160, 20);
        ofDrawBitmapString(sentString, sentPos);
        ofPopStyle();
        
        
        ofVec2f statusPos(leftMargin + 170, topMargin + 130);
        
        string statusString;
        ofColor statusCol;
        
        if( appStatus == 0 ){
            statusString = "System stabilizing";
            statusCol.set(255, 150, 0);
            
        } else if( appStatus == 1 ){
            statusString = "System OK";
            statusCol.set(0, 255, 0);
        
        } else if( appStatus == 2 ){
            statusString = "System NOT OK";
            statusCol.set(255, 0, 0);
        }
        
        
        ofPushStyle();
        ofNoFill();
        ofSetColor(statusCol);
        ofDrawRectangle(statusPos.x - 5, statusPos.y - 15, 160, 20);
        ofDrawBitmapString(statusString, statusPos);
        ofPopStyle();
        
    } else if( currentView == ALL_CAMS ){
        
        
        title = "All Camera View";
        
        
        ofPushMatrix();
        ofTranslate(leftMargin, topMargin);
        
        //Make it bigger and easier to see
        ofScale( 1.3, 1.3 );
        
        float margin = 20;
        
        for (int i = 0; i < feeds.size(); i++){
            
            float x = (camWidth + margin) * (i % 3);
            float y;
            if( i < 3 ){
                y = 0;
            } else if( i < 6 ){
                y = camHeight + margin;
            } else {
                y = camHeight*2 + margin*2;
            }
            
            
            feeds[i].drawRaw(x, y);
            
        }
        
        ofPopMatrix();
        
        
    } else if( currentView >= CAMS_0_3 && currentView <= CAMS_4_7 ){
        
        //----------CAMERAS VIEW----------
        int firstCam = (currentView == CAMS_0_3) ? 0 : 4;
        int lastCam = (currentView == CAMS_0_3) ? 3 : 6;
        
        title = "Cameras " + ofToString(firstCam)+ " to " +ofToString(lastCam);

        slot1.set(leftMargin, topMargin);
        slot2.set(leftMargin + camWidth*2 + gutter*2, topMargin);
        slot3.set(leftMargin, topMargin + camHeight*2 + 40);
        slot4.set(leftMargin + camWidth*2 + gutter*2, topMargin + camHeight*2 + 40);
        
        for( int i = firstCam; i <= lastCam; i++){
            
            ofVec2f place;
            if ( i == firstCam ){
                place = slot1;
            } else if( i == firstCam + 1){
                place = slot2;
            } else if( i == firstCam + 2){
                place = slot3;
            } else if( i == firstCam + 3){
                place = slot4;
            }
            
            feeds[i].drawRawAndProcessed(place.x, place.y);
            
            //draw pixel histogram right below it (+ 20 pixels)
            feeds[i].pixelStats.drawDistribution(place.x, place.y + camHeight + 20, camWidth*2, 150 );
            
        }
        
        

        
    } else if( currentView == STITCHING ){
        
        
        //----------STITCHING VIEW----------
        
        title = "Stitched Composite View";

        string s = "Composite Dimensions: " + ofToString(masterWidth) + " x " + ofToString(masterHeight);
        
        ofDrawBitmapString(s, detectionDisplayPos.x, detectionDisplayPos.y - 5);
        
        ofPushMatrix();{
            
            ofTranslate(detectionDisplayPos);
            ofScale(compositeDisplayScale, compositeDisplayScale);
            
            drawMasterComposite(0, 0);
            
            
        }ofPopMatrix();
        

        //cam repositioning warning
        ofSetColor(255, 0, 0);
        smallerFont.drawString("NOTE:", leftMargin, detectionDisplayPos.y + compositeDisplayScale*masterHeight + smallerFont.stringHeight("Ag")*2);
        
        ofSetColor(255);
        string warning = "";
        warning += "Learned background subtraction is cleared whenever the\n";
        warning += "composite image's dimensions are changed.";
//        warning += "\n";
//        warning += "Also, 7th Cam automatically positions itself to the far\n";
//        warning += "right corner of the aggregate.\n";
        
        smallerFont.drawString(warning, leftMargin, detectionDisplayPos.y + compositeDisplayScale*masterHeight + smallerFont.stringHeight("Ag")*3 + 5);
        
        //draw gui
        stitchingGui.setPosition(stitchingGuiPos -> x, stitchingGuiPos -> y);
        stitchingGui.draw();
        
        
    } else if( currentView == MASKING ){
        
        //----------MASKING VIEW----------
        title = "Mask Editing";
        
        
        //draw the Stitched and Processed view, before
        
        
        ofImage img;
        img.setFromPixels(foregroundPix.getData(), foregroundPix.getWidth(), foregroundPix.getHeight(), OF_IMAGE_GRAYSCALE);
        ofSetColor(255);
        img.draw(maskScreenPos);

        ofSetColor(maskCol);
        ofDrawBitmapString("Draw the desired mask into the raw image:", maskScreenPos.x, maskScreenPos.y - 5);

        ofNoFill();
        ofDrawRectangle(maskScreenPos, masterWidth, masterHeight);

        string note = "";
        note += "Masking\n";
        note += "-------\n";
        note += "Toggle 'Draw or Erase' to draw\n";
        note += "into mask or erase mask pixels.\n";
        note += "\n";
        note += "Clear mas  k to erase everything.\n";
        note += "Save/Load to and from png in data folder\n";
        
        ofSetColor(maskCol);
        ofDrawBitmapString(note, maskScreenPos.x + masterPix.getWidth() + 10, maskScreenPos.y + 10);
        
        
        //draw the mask
        ofSetColor(maskCol, 100);
        maskImg.setFromPixels(maskPix);
        maskImg.draw(maskScreenPos);
        
        //draw the foreground too for comparison with the mask
        ofSetColor(255);
        ofDrawBitmapString("Foreground", maskScreenPos.x, maskScreenPos.y - 5 + masterHeight + gutter);
        img.setFromPixels(foregroundPix.getData(), foregroundPix.getWidth(), foregroundPix.getHeight(), OF_IMAGE_GRAYSCALE);
        img.draw(maskScreenPos.x, maskScreenPos.y + masterHeight + gutter);

        
        ofPushStyle();
        ofPushMatrix();
        
        ofTranslate(maskScreenPos.x, maskScreenPos.y + masterHeight + gutter);
        
        ofSetLineWidth(2.0);
        ofSetColor(255, 0, 0);
        contours.draw();

        
        ofPopStyle();
        ofPopMatrix();
        
        ofNoFill();
        ofDrawRectangle(maskScreenPos.x, maskScreenPos.y + masterHeight + gutter, masterWidth, masterHeight);
        
        
        if( bMouseInsideMask && useMask ){
            ofPushStyle();
            
            //mask cursor fill
            if(drawOrErase){
                
                //fill with color
                ofFill();
                ofSetColor(maskCol);
                ofDrawRectangle(ofGetMouseX() - maskToolSize/2, ofGetMouseY() - maskToolSize/2, maskToolSize, maskToolSize);
                
            } else {

                //draw little red X centered at mouse pos
                ofSetLineWidth(2);
                int l = maskToolSize/2;
                ofPushMatrix();
                ofTranslate(ofGetMouseX(), ofGetMouseY());
                ofSetColor(255, 0, 0);
                ofDrawLine(-l, -l, l, l);
                ofDrawLine(l, -l, -l, l);
                ofPopMatrix();
                
            }

            //mask cursor outline
            ofNoFill();
            ofSetLineWidth(1);
            ofSetColor(255);
            ofDrawRectangle(ofGetMouseX() - maskToolSize/2, ofGetMouseY() - maskToolSize/2, maskToolSize, maskToolSize);
            
            ofPopStyle();
            
        }
        
        //draw gui
        maskingGui.setPosition(maskGuiPos -> x, maskGuiPos -> y);
        maskingGui.draw();
        
        
        
    } else if( currentView == PIPELINE ){
        
        //----------PIPELINE VIEW----------
        title = "CV Pipeline";
        
        //adjust the content positions to reflect the changed position of the
        //composite image
        slot1.set(0                   , 0);
        slot2.set(masterWidth + gutter, 0);
        slot3.set(0                   , masterHeight + gutter);
        slot4.set(masterWidth + gutter, masterHeight + gutter);
//        slot5.set(0                   , masterHeight*2 + gutter*2);
//        slot6.set(masterWidth + gutter, masterHeight*2 + gutter*2);
        
        ofPushMatrix();{
            
            ofTranslate(leftMargin, topMargin);
            ofScale(pipelineDisplayScale, pipelineDisplayScale);

            ofImage img;
            
            //----------slot 1----------
            ofSetColor(255);
            ofDrawBitmapString("Stitched & Processed", slot1.x, slot1.y - 5);
            img.setFromPixels(masterPix.getData(), masterPix.getWidth(), masterPix.getHeight(), OF_IMAGE_GRAYSCALE);
            img.draw(slot1);
            
            ofNoFill();
            ofDrawRectangle(slot1, masterWidth, masterHeight);
            
            //----------slot 2----------
            ofSetColor(255);
            ofDrawBitmapString("Subtracted Background", slot2.x, slot2.y - 5);
            img.setFromPixels(backgroundPix.getData(), backgroundPix.getWidth(), backgroundPix.getHeight(), OF_IMAGE_GRAYSCALE);
            img.draw(slot2);
            
            ofNoFill();
            ofDrawRectangle(slot2, masterWidth, masterHeight);
            
            //----------slot 3----------
            ofDrawBitmapString("Foreground", slot3.x, slot3.y - 5);
            img.setFromPixels(foregroundPix.getData(), foregroundPix.getWidth(), foregroundPix.getHeight(), OF_IMAGE_GRAYSCALE);
            img.draw(slot3);
            
            ofNoFill();
            ofDrawRectangle(slot3, masterWidth, masterHeight);
            
            //----------slot 4----------
            ofDrawBitmapString("Thresholded", slot4.x, slot4.y - 5);
            img.setFromPixels(threshPix.getData(), threshPix.getWidth(), threshPix.getHeight(), OF_IMAGE_GRAYSCALE);
            img.draw(slot4);
            
            ofNoFill();
            ofDrawRectangle(slot4, masterWidth, masterHeight);
            
            

            
        }ofPopMatrix();
        
        //draw contours over thresholded image in slot 4
        if(drawContoursToggle){
            
            ofPushMatrix();
            ofTranslate(leftMargin + slot4.x*pipelineDisplayScale, topMargin + slot4.y*pipelineDisplayScale);
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

        
        

        
        
                
        
    } else if( currentView == ZONES ){
        
        //----------DETECTION ZONE VIEW----------
        title = "Detection Zones";
        
        
        //----------Primary Slot----------
        ofDrawBitmapString("Contours and Detection zones", detectionDisplayPos.x, detectionDisplayPos.y - 5);
        
        ofPushMatrix();{
            
            ofTranslate(detectionDisplayPos);
            ofScale(compositeDisplayScale, compositeDisplayScale);
            
            bool bDrawIDs = true;
            bool bUseColors = false;
            bool bDrawRawFeed = false;
            drawMasterComposite(0, 0, bDrawIDs, bUseColors, bDrawRawFeed);
            
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
                    
                    //if we're in zone 1, subtract from it zone 0
                    if( activeZone == 1 ){
                        slice.close();
                        slice.append(zones[0].path);
                    }
                    
                    slice.draw();
                    
                }
                
                
                for(int i = zones.size() - 1; i >= 0 ; i--){
                    //draw, but tell it how much it's being scaled by
                    zones[i].draw(compositeDisplayScale);
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
        
        string sentString;
        float t = ofMap(ofGetElapsedTimef() - lastStatusSendTime, 0, 0.1, 255, 100, true);
        
        if( sendOSCToggle ){
            ofSetColor(0, 255, 0, t);
            sentString = "OSC MESSAGE SENT...";
        } else {
            ofSetColor(255, 0, 0);
            sentString = "OSC NOT SENDING";
        }
        
        ofVec2f sentPos(detectionDisplayPos.x + 170, detectionDisplayPos.y + ( masterHeight * compositeDisplayScale) + 30);
        
        ofDrawBitmapString(sentString, sentPos);
        
        ofPushStyle();
        ofNoFill();
        ofDrawRectangle(sentPos.x - 5, sentPos.y - 15, 160, 20);
        ofPopStyle();
        
        
    } else if( currentView == ADDRESSING ){
        
        title = "Camera Address Management";
        
        //draw composite as a reference
        drawMasterComposite(leftMargin, topMargin);
        
        string helpText = "";
        
        helpText += "App automatically pulls cameras from file. If new hardware arrangement\n";
        helpText += "changes camera addresses, click \"Reset Addresses\" button at left.\n";
        helpText += "Feeds will populate with frames and IDs based on the order they come in.\n";
        helpText += "\n";
        helpText += "Once frames from all 6 cameras have been received, select an \n";
        helpText += "address and use the arrows to to change their order.\n";
        
        ofSetColor(255);
        ofDrawBitmapString(helpText, leftMargin, topMargin + masterHeight + 40);
        
        
        //draw the address panel below it
        addressPanel.draw(leftMargin, topMargin + masterHeight + 40 + 15*6);
        //6 lines of help text, 15 pixels each
        
        
        
        
        
        
        
    }
    
    ofSetColor(255);
    titleFont.drawString(title, titlePos.x, titlePos.y);
    
    ofSetColor(255);
    ofDrawBitmapString("Framerate: " + ofToString(ofGetFrameRate()), 10, 15);
    
    
    
    
    
    string keyInfo = "";
    
    keyInfo += "Key Bindings\n";
    keyInfo += "------------\n";
    keyInfo += "'S' to Save 'L' to Load\n";
    keyInfo += "Left/Right or [#] to\n";
    keyInfo += "switch between views:\n";
    keyInfo += "0 - \"Headless\" View\n";
    keyInfo += "1 - All Cam View\n";
    keyInfo += "2 - Cameras 0 - 3\n";
    keyInfo += "3 - Cameras 4 - 7\n";
    keyInfo += "4 - Masking view\n";
    keyInfo += "5 - Stitching view\n";
    keyInfo += "6 - CV Pipeline\n";
    keyInfo += "7 - Detection Zones\n";
    keyInfo += "8 - Camera Addressing\n";
    
    ofDrawBitmapString(keyInfo, 10, ofGetHeight() - 195);
    
    
    if( currentView != HEADLESS ){
        drawGui(10, 20);
    }
    
    
    
    //box with save/load feedback
    drawSaveLoadBox();
    
    
}


void ofApp::drawMasterComposite(int x, int y, bool bDrawIDs, bool bUseColors, bool bDrawRaw){
    
    ofPushStyle();
    ofPushMatrix();{
    ofTranslate(x, y);
    
        ofImage img;
        
        ofSetColor(255);
        ofSetLineWidth(1);
        
        //if we're drawing the raw image, use masterPix, if not, use the foregroundPix
        if( bDrawRaw ){
            img.setFromPixels(masterPix.getData(), masterWidth, masterHeight, OF_IMAGE_GRAYSCALE);
        } else {
            img.setFromPixels(foregroundPix.getData(), masterWidth, masterHeight, OF_IMAGE_GRAYSCALE);
        }
        
        img.draw(0, 0);
        
        ofNoFill();
        ofDrawRectangle(0, 0, masterWidth, masterHeight);
        
        //draw outlines of individual cameras and titles within the composite image
        for( int i = 0; i < TOTAL_NUM_CAMS; i++){
            
            ofColor c;
            
            if( bUseColors ){
                c.setHsb( i * 255/TOTAL_NUM_CAMS, 200, 200);
            } else {
                c.set(255, 70);
            }
            
            ofSetColor(c);
            
            if( bDrawIDs ){
                string text = "Cam " + ofToString(i) + "\nID: " + ofToString(feeds[i].camID);
                ofDrawBitmapString(text, camPositions[i] -> x + 5, camPositions[i] -> y + 10);
            }
            
            ofDrawRectangle(camPositions[i] -> x, camPositions[i] -> y, camRotations[i] % 2 == 1 ? camHeight : camWidth, camRotations[i] % 2 == 1 ? camWidth : camHeight);
            
            //draw X if frame is being dropped from bad data
            if( feeds[i].bDropThisFrame ){
                
                ofSetColor(255, 0, 0);
                ofSetLineWidth(1.5);
                
                ofVec2f p(camPositions[i] -> x, camPositions[i] -> y);
                int w = camRotations[i] % 2 == 1 ? camHeight : camWidth;
                int h = camRotations[i] % 2 == 1 ? camWidth : camHeight;
                
                ofDrawLine(p.x, p.y, p.x + w, p.y + h);
                ofDrawLine(p.x + w, p.y, p.x, p.y + h);
                
            }
            
            
        }
        
        
    }ofPopMatrix();
    ofPopStyle();
    
    
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
    

    //#1 key goes to view 0, etc...
    if( key == '0' ){
        currentView = 0;
    } else if( key == '1' ){
        currentView = 1;
    } else if( key == '2' ){
        currentView = 2;
    } else if( key == '3' ){
        currentView = 3;
    } else if( key == '4' ){
        currentView = 4;
    } else if( key == '5' ){
        currentView = 5;
    } else if( key == '6' ){
        currentView = 6;
    } else if( key == '7' ){
        currentView = 7;
    } else if( key == '8' ){
        currentView = 8;
    }
//    else if( key == '9' ){
//        currentView = 9;
//    }
    
    
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
    
    if( currentView == ZONES ){
        
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
    
    if( currentView == ADDRESSING ){
        addressPanel.checkForMouseClicks(x, y);
    }
    

//    cout << x << ", " << y << endl;
    
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
    
    gui.add(OSCLabel.setup("   OSC SETTINGS", ""));
    gui.add(sendOSCToggle.setup("Send OSC Data", false));
    gui.add(waitBeforeOSCSlider.setup("Wait after startup", 7.0, 5.0, 30));
    gui.add(sysNotOKSlider.setup("Time for NOT OK Flag", 4.0, 1.0, 20));
    gui.add(maxOSCSendRate.setup("Zones interval (s)", 0.5f, 0.0f, 2.0f));
    gui.add(statusSendRate.setup("Status interval (s)", 0.5f, 0.0f, 2.0f));
    
    gui.add(addressingLabel.setup("   CAM ADDRESSING", ""));
    gui.add(resetCamAddresses.setup("Reset Address", false));
    
    
    
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
//    gui2.add(active3Pt0.setup("Active Z-3 Pt 0", start, start, end));
//    gui2.add(active3Pt1.setup("Active Z-3 Pt 1", start, start, end));
//    gui2.add(active3Pt2.setup("Active Z-3 Pt 2", start, start, end));
//    gui2.add(active3Pt3.setup("Active Z-3 Pt 3", start, start, end));
    
    //    gui2.add(stitchingPointsLabel.setup("   STITCHING POSITIONS", ""));
    
    
    gui2.minimizeAll();
    
    stitchingGuiName = "stitchingGui";
    stitchingGui.setup(stitchingGuiName, stitchingGuiName + ".xml", 0, 0);
    
    stitchingGui.add( stitchingGuiPos.setup("Gui Pos", ofVec2f(200, 50), ofVec2f(0, 0), ofVec2f(ofGetWidth(), ofGetHeight())));
    stitchingGui.add(trimMasterPixButton.setup("Trim pixels"));
    
    stitchingGui.add(stitchingLabel.setup("   CAMERA STITCHING", ""));
    
    
    start.set(0, 0);
    end.set(camWidth*3, camHeight);

    for(int i = 0; i < TOTAL_NUM_CAMS; i++){
        stitchingGui.add(camPositions[i].setup("Cam " +ofToString(i)+ " Position", ofVec2f(0, 0), start, end));
        stitchingGui.add(camRotations[i].setup("Cam " +ofToString(i)+ " Rotation", 0, 0, 3));
        stitchingGui.add(camMirrorToggles[i].setup("Cam " +ofToString(i)+ " Mirror", false));
    }

    stitchingGui.minimizeAll();
    
    
    
    maskGuiName = "maskingGui";
    maskingGui.setup(maskGuiName, maskGuiName + ".xml", 0, 0);
    maskingGui.add( maskGuiPos.setup("Gui Pos", ofVec2f(200, 50), ofVec2f(0, 0), ofVec2f(ofGetWidth(), ofGetHeight())));

    maskingGui.add(useMask.setup("Use Mask", true));
    maskingGui.add(drawOrErase.setup("Draw or Erase", true));
    maskingGui.add(clearMask.setup("Clear Mask"));
    maskingGui.add(saveMask.setup("Save Mask"));
    maskingGui.add(loadMask.setup("Load mask"));
    maskingGui.add(maskToolSize.setup("Mask Tool Size", 15, 4, 50));
    
    
    maskingGui.setHeaderBackgroundColor(ofColor(255));
    maskingGui.minimizeAll();
    
    //color applies to gui title only
    maskingGui.setTextColor(ofColor(0));
    

    
    //-----GUI 1 formatting-----
    gui.setHeaderBackgroundColor(ofColor(255));
    
    //color applies to gui title only
    gui.setTextColor(ofColor(0));
    
    imageAdjustLabel.setBackgroundColor(ofColor(255));
    bgDiffLabel.setBackgroundColor(ofColor(255));
    contoursLabel.setBackgroundColor(ofColor(255));
    OSCLabel.setBackgroundColor(ofColor(255));
    addressingLabel.setBackgroundColor(ofColor(255));
    
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
    
    maskingGui.loadFromFile(maskGuiName + ".xml");
    maskingGui.setPosition( maskGuiPos -> x, maskGuiPos -> y );
    
    //load the mask from file or prepare one if it hasn't been found
    maskFileName = "mask/mask.png";
    
    if(maskImg.load(maskFileName)){
        
        cout << "Mask loaded" << endl;
        maskPix = maskImg.getPixels();
        
    } else {
        
        cout << "Mask not found, creating: " << masterWidth << ", " << masterHeight << endl;
        
        maskPix.allocate(masterWidth, masterHeight, OF_IMAGE_GRAYSCALE);
        maskPix.setColor(ofColor(0));
        
        maskImg.setFromPixels(maskPix);
        maskImg.update();
        maskImg.save(maskFileName);
        
    }

    
    addressFilename = "camAddresses.txt";
    
    ofBuffer addressFileBuffer = ofBufferFromFile(addressFilename);
    
    addresses.clear();
    addresses.assign(TOTAL_NUM_CAMS, 0);
    
    if(addressFileBuffer.size()) {
        
        int lineNum = 0;
        
        for (ofBuffer::Line it = addressFileBuffer.getLines().begin(), end = addressFileBuffer.getLines().end(); it != end; ++it) {
            
            string line = *it;
            addresses[lineNum] = ofToInt(line);
            
            cout << "Addresses[" + ofToString(lineNum) + "]: " << addresses[lineNum] << endl;
            
            lineNum++;
            
        }
        
    }
    
    
}

void ofApp::saveSettings(){
    
    gui.saveToFile(guiName + ".xml");
    gui2.saveToFile(gui2Name + ".xml");
    stitchingGui.saveToFile(stitchingGuiName + ".xml");
    maskingGui.saveToFile(maskGuiName + ".xml");

    //save the mask too
    maskImg.setFromPixels(maskPix);
    maskImg.save(maskFileName);
    
    
    
    //save the addresses to file
    //also create a new text file to store it for next time
    ofBuffer buffer;
    
    
    for(int i = 0; i < addresses.size(); i++){

        buffer.append( ofToString(addresses[i]) );
        
        //don't add the line break on the last line
        if( i < addresses.size() - 1){
            buffer.append("\n");
        }
        
    }
    
    ofBufferToFile(addressFilename, buffer);
    
    
    
}

void ofApp::drawGui(int x, int y){
    
    gui.setPosition(x, y);
    gui.draw();
    
//    if( showSecondGui ){
//        gui2.setPosition(gui2Pos -> x, gui2Pos -> y);
//        gui2.draw();
//    }
    

    
    
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


