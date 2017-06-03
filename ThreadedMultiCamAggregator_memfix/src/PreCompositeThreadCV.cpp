//
//  PreCompositeThreadCV.cpp
//  Single_Room_Aggregator
//
//  Created by Adiel Fernandez on 5/15/17.
//
//

#include "PreCompositeThreadCV.hpp"


PreCompositeThreadCV::PreCompositeThreadCV(){
    
}


PreCompositeThreadCV::~PreCompositeThreadCV(){
    
    
    closeAllChannels();
    
    waitForThread(true, 4000);
    
}

void PreCompositeThreadCV::closeAllChannels(){
    
    //close thread channels
    //    rawPix_IN.close();
    //    settings_IN.close();
    
    newFrame_IN.close();
    newPix_OUT.close();
    
}

void PreCompositeThreadCV::emptyAllChannels(){
    
    cout << "Emptying all thread channels" << endl;
    

    
    newFrame_IN.empty();
    newPix_OUT.empty();
    
}

void PreCompositeThreadCV::setup(ofPixels *_mainPix){
    
    //get pointers to the objects on the main thread we'll be filling
    mainPix = _mainPix;
    
    
    //Thread management and background resetting
    bIsThreadCrashed = false;
    bIsNewlyCrashed = true;
    bIsNewlyStarted = true;
    lastRestartTime = 0;
    
    startThread();
    
}

//threadChannel's send() method already makes a copy so the
//parameters of analyze() are references to avoid double copies
void PreCompositeThreadCV::analyze(ofPixels & p, vector<int> & settings){
    
    NewFrame newF;
    newF.pix = p;
    newF.settings = settings;
    
    newFrame_IN.send(newF);
    
}

void PreCompositeThreadCV::update(){
    
    //attempt to receive data from thread
    ofPixels t;
    if(newPix_OUT.tryReceive(t)){
        *mainPix = t;
        
//        cout << "Num channels in thread output" << mainPix -> getNumChannels() << endl;
//        cout << "New frame from thread" << endl;
    }

    
    
    //---------------------------------------------------------------
    //---------------THREAD MANAGEMENT AND RESTARTING----------------
    //---------------------------------------------------------------
    
    //if it's been 6 seconds since w'eve last received a frame,
    //consider the thread crashed
//    if(ofGetElapsedTimef() - frameCount.lastFrameTime > 5.0f){
//        bIsThreadCrashed = true;
//        
//        if(bIsNewlyCrashed){
//            cout << "[thread] Stopping Thread" << endl;
//            stopThread();
//            
//            emptyAllChannels();
//            
//            bIsNewlyCrashed = false;
//        }
//        
//    } else {
//        
//        bIsThreadCrashed = false;
//        
//        //set this to true so it's ready in case a thread does crash
//        bIsNewlyCrashed = true;
//        
//    }
//    
//    
//    //only try to restart the thread every 4 seconds
//    if(bIsThreadCrashed && ofGetElapsedTimef() - lastRestartTime > 4.0f){
//        
//        cout << "[thread] Attempting to start thread..." << endl;
//        startThread();
//        
//        lastRestartTime = ofGetElapsedTimef();
//        bIsNewlyStarted = true;
//        
//    }
//    
//    //if it has been 2 seconds since last restart AND we're still crashed
//    //then stop and prepare for the next restart
//    //make sure to wait longer since we have waitForThread(4000)
//    if(bIsThreadCrashed && bIsNewlyStarted && ofGetElapsedTimef() - lastRestartTime > 3.0f){
//        
//        cout << "[thread] Re-Stopping Thread" << endl;
//        stopThread();
//        
//        emptyAllChannels();
//        closeAllChannels();
//        
//        bIsNewlyStarted = false;
//        
//    }
    
    
    
    
    
}



void PreCompositeThreadCV::threadedFunction(){
    
    while(isThreadRunning()){
        
        //wait until we have our settings AND our pixels loaded
        //        if(settings_IN.receive(settings_thread) && rawPix_IN.receive(incomingPix_thread)){
        
        if(newFrame_IN.receive(nf)){
            
            //unpack the vector and save it to the values we'll be using
            int blurAmt = nf.settings[0];
            float contrastExp = nf.settings[1]/1000.0f;   //divide to cast int to float
            float contrastShift = nf.settings[2]/1000.0f; //divide to cast int to float
            
            
            nf.pix.setImageType(OF_IMAGE_GRAYSCALE);
            
            ofxCv::GaussianBlur(nf.pix, blurAmt);
            
            //Contrast adjustment
            for(int i = 0; i < nf.pix.getWidth() * nf.pix.getHeight(); i++){
                
                float normPixVal = nf.pix[i]/255.0f;
                
                nf.pix[i] = ofClamp( 255 * pow((normPixVal + contrastShift), contrastExp), 0, 255);
            }
                        
            
            //send things out to the GL-thread
            newPix_OUT.send(std::move(nf.pix));
            
        }
        
        
        
    }
    
}
