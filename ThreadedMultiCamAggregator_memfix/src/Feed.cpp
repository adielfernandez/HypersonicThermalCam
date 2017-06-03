//
//  Feed.cpp
//  ThreadedMultiCamAggregator
//
//  Created by Adiel Fernandez on 4/7/17.
//
//

#include "Feed.hpp"


Feed::Feed(){
    
}

Feed::Feed(const Feed &f){
    
}

void Feed::setup(int num, int _id, int w, int h){
    
    camNum = num;
    camID = _id;
    camWidth = w;
    camHeight = h;
    
    rawImg.allocate(camWidth, camHeight, OF_IMAGE_COLOR_ALPHA);
//    rawPix.allocate(camWidth, camHeight, OF_IMAGE_COLOR_ALPHA);
    grayPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    
    
    blackPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    blackPix.setColor(0);
    
    pixelStats.setup(camNum);
    bDropThisFrame = false;
    

    img.setFromPixels(blackPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    
    //give reference to the pixel object the thread will fill
    threadedCV.setup( &grayPix );
    
}


void Feed::newFrame( ofPixels &raw ){

    rawImg.getPixels() = raw;
    rawImg.update();
    
//    rawPix = raw;
//    grayPix = gray;
    
    
    vector<int> settings;
    settings.resize(3);
    settings[0] = *blurAmt;
    settings[1] = (*contrastExp) * 1000;
    settings[2] = (*contrastPhase) * 1000;

    
    //tell the thread to analyze the frame
    //ofApp will update the thread and that will fill grayPix
    threadedCV.analyze( raw, settings );
    
//    adjustContrast( &grayPix , (*contrastExp), (*contrastPhase) );

    
    
    //store some framerate data
    float thisFrameRate = 1.0/( (ofGetElapsedTimef() - lastFrameTime) );
    
    //log frame rates for each camera and average with the
    //last recorded frame rate to smooth a little
    camFrameRate = (thisFrameRate + lastFrameRate)/2;
    lastFrameRate = thisFrameRate;
    lastFrameTime = ofGetElapsedTimef();

    
//    cout << "Cam " << " last frame time: " << lastFrameTime << endl;
    
    if( *stdDevToggle ){
        
        pixelStats.setStdDevThresh( *stdDevThresh );
        pixelStats.setAvgPixThresh( *avgPixThresh );
        pixelStats.analyze( &grayPix );
        
        if( pixelStats.bDataIsBad ){
            bDropThisFrame = true;
        } else {
            bDropThisFrame = false;
        }
        
    } else {
        bDropThisFrame = false;
    }
        
    
    
    
}


void Feed::update(){
    
    timeSinceLastFrame = ofGetElapsedTimef() - lastFrameTime;
    
    threadedCV.update();
    
}



ofPixels Feed::getOutputPix(){
    
    if( bDropThisFrame ){
        return blackPix;
    } else {
        return grayPix;
    }
    
}

void Feed::resetAllPixels(){
    
//    rawPix = blackPix;
    rawImg.getPixels() = blackPix;
    rawImg.update();
    
    grayPix = blackPix;
    
    img.setFromPixels(blackPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    
}

void Feed::drawRaw( int x, int y ){
    
    ofPushStyle();
    
    //DONT call ofSetColor here, we'll do it from ofApp using gui settings
    //ofSetColor(255);
    
    
    ofSetLineWidth(1);
    
    
    
    string fr;
    
    if ( timeSinceLastFrame > 2.0f ) {
        fr =  "---";
        ofSetColor(255, 0, 0);
    } else {
        fr = ofToString(camFrameRate, 2);
        ofSetColor(255);
    }
    
    ofDrawBitmapString("Cam " + ofToString(camNum) + " FR: "  + fr + "\nTime since last frame: " + ofToString(timeSinceLastFrame), x, y-5);

    
//    img.setFromPixels(rawPix.getData(), camWidth, camHeight, OF_IMAGE_COLOR_ALPHA);
//    img.draw(x, y);

    rawImg.draw(x, y + 10);
    
    ofNoFill();
    ofDrawRectangle(x, y + 10, camWidth, camHeight);
    
    ofPopStyle();

    
    
    
}


void Feed::drawRawAndProcessed(int x, int y){
    
    ofPushStyle();
    
    ofSetColor(255);
    ofSetLineWidth(1);
    
    string fr =  timeSinceLastFrame > 3.0f ? "---" : ofToString(camFrameRate, 2);
    ofDrawBitmapString("Cam " + ofToString(camNum) + " FR: "  + fr + ", Time since last frame: " + ofToString(timeSinceLastFrame), x, y-5);
    
    
    rawImg.draw(x, y);
    
//    if( rawPix.isAllocated() ){
//        img.setFromPixels(rawPix.getData(), camWidth, camHeight, OF_IMAGE_COLOR_ALPHA);
//        img.draw(x, y);
//    }
    
    if( grayPix.isAllocated() ){
        img.setFromPixels(grayPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
        img.draw(x + camWidth, y);        
    }
    
    ofNoFill();
    ofDrawRectangle(x, y, camWidth * 2, camHeight);
    
    
    //draw X over the second frame
    if( bDropThisFrame ){
        
        ofSetColor(255, 0, 0);
        ofSetLineWidth(3);

        ofDrawLine(x + camWidth, y, x + camWidth*2, y + camHeight);
        ofDrawLine(x + camWidth, y + camHeight, x + camWidth*2, y);
        
    }
    
    ofPopStyle();
    
    
}




void Feed::adjustContrast( ofPixels *pix, float exp, float phase){
    
    for(int i = 0; i < pix -> getWidth() * pix -> getHeight(); i++){
        //normalized pixel value
        float normPixVal = (*pix)[i]/255.0f;
        (*pix)[i] = ofClamp( 255 * pow((normPixVal + phase), exp), 0, 255);
    }
    
}
