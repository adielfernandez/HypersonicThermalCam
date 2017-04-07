//
//  Aggregator.cpp
//  ThreadedMultiCamAggregator
//
//  Created by Adiel Fernandez on 4/7/17.
//
//

#include "Aggregator.hpp"

using namespace ofxCv;

Aggregator::Aggregator(){
    
    closeAllChannels();
    
    waitForThread(true, 4000);
    
}

void Aggregator::closeAllChannels(){
    
    settingsIn.close();
    rawPixIn.close();
    threshPixOut.close();
    contoursOut.close();
    
}


void Aggregator::emptyAllChannels(){
    
    //close thread channels
    settingsIn.empty();
    rawPixIn.empty();
    threshPixOut.empty();
    contoursOut.empty();
    
}

void Aggregator::setup(int singleW, int singleH){

    camWidth = singleW;
    camHeight = singleH;
    
    frameNum = 0;
    
    camFrameRate = 0;
    lastFrameTime = 0;
    
    needsAutoReset = true;
    isThreadCrashed = false;
    firstAfterCrash = true;
    
    firstRestart = true;
    
    lastRestartTime = 0;
    
    processedPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    threshPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    backgroundPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    foregroundPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    
    startThread();
    
    //Pretend we just got a frame so the thread doesn't stop
    //thinking it crashed since ofGetElapsedTimeMillis starts
    //long before the window opens (assets take a long time to load)
    lastFrameTime = ofGetElapsedTimeMillis();
    
}



void Aggregator::update(){
    
    //attempt to receive data from thread
//    ofPixels p;
//    if(rawPixOut.tryReceive(p)){
//        rawPix = p;
//    }
//    
//    ofPixels t;
//    if(threshPixOut.tryReceive(t)){
//        threshPix = t;
//        
//        float thisFrameRate = 1.0/( (ofGetElapsedTimeMillis() - lastFrameTime) / 1000.0 );
//        
//        //average this framerate with the last one to smooth out numbers
//        //and get a better reading.
//        camFrameRate = (thisFrameRate + lastFrameRate)/2;
//        lastFrameRate = thisFrameRate;
//        
//        //
//        lastFrameTime = ofGetElapsedTimeMillis();
//        
//    }
//    
//    ofxCv::ContourFinder c;
//    if(contoursOut.tryReceive(c)){
//        contours = c;
//        
//        bNewContours = true;
//    }
//    
//    ofPixels fg;
//    if(foregroundPixOut.tryReceive(fg)){
//        foregroundPix = fg;
//    }
    
    
    
    
    //---------------------------------------------------------------
    //---------------THREAD MANAGEMENT AND RESTARTING----------------
    //---------------------------------------------------------------
    
    if(ofGetElapsedTimeMillis() - lastFrameTime > 6000){
        isThreadCrashed = true;
        
        if(firstAfterCrash){
            cout << "Stopping Aggregator Thread" << endl;
            stopThread();
            emptyAllChannels();
            background.reset();
            firstAfterCrash = false;
        }
        
    } else {
        isThreadCrashed = false;
        firstAfterCrash = true;
    }
    
    
    //only try to restart the thread every 4 seconds
    if(isThreadCrashed && ofGetElapsedTimeMillis() - lastRestartTime > 4000){
        
        cout << "Attempting to start Aggregator thread..." << endl;
        startThread();
        
        lastRestartTime = ofGetElapsedTimeMillis();
        firstRestart = true;
        
    }
    
    //if it has been 2 seconds since last restart AND we're still crashed
    //then stop and prepare for the next restart
    //make sure to wait longer since we have waitForThread(4000)
    if(isThreadCrashed && ofGetElapsedTimeMillis() - lastRestartTime > 3000 && firstRestart){
        
        cout << "Stopping Aggregator Thread" << endl;
        stopThread();
        
        emptyAllChannels();
        closeAllChannels();
        
        background.reset();
        
        firstRestart = false;
        
    }
    
    
}
    
        
void Aggregator::analyze(ofPixels *pix, vector<int> settings){
    
    //send settings into thread
    rawPixIn.send( (*pix) );
    
    //send in gui settings (pre-packaged from ofApp)
    settingsIn.send(settings);
    
}



void Aggregator::threadedFunction(){
    
    while(isThreadRunning()){
        
        vector<int> settings_thread;
        ofPixels rawPix_thread;
        
        if(rawPixIn.receive(rawPix_thread) && settingsIn.receive(settings_thread)){
        
        
            
            
            
            
        
        
        
        }
        
    }
    
}



