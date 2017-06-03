//
//  PreCompositeThreadCV.hpp
//  Single_Room_Aggregator
//
//  Created by Adiel Fernandez on 5/15/17.
//
//

#ifndef PreCompositeThreadCV_hpp
#define PreCompositeThreadCV_hpp

#include <stdio.h>

#endif /* PreCompositeThreadCV_hpp */

#include "ofMain.h"
#include "ofxCv.h"
#pragma once


/*
 * PRECompositeThreadCV:
 *  INPUT:
 *      -Raw Pixels (post quad mapping)
 *      -CV variables:
 *          -Blur amt, threshold, etc.
 *
 *  OUTPUT:
 *      -altered Pixel object
 */

class PreCompositeThreadCV: public ofThread{
    
public:
    
    PreCompositeThreadCV();
    ~PreCompositeThreadCV();
    
    void setup(ofPixels *_mainPix);
    void analyze(ofPixels & pix, vector<int> & settings);
    void closeAllChannels();
    void emptyAllChannels();
    
    void update();
    
    struct NewFrame{
        ofPixels pix;
        vector<int> settings;
    };
    
    
    //pointers to the objects that will use
    //the 'PostCompositeThreadCV' class. We'll fill them
    //directly when getting things back from the thread
    ofPixels *mainPix;
    
    
    //for restarting the thread
    float lastRestartTime;
    bool bIsThreadCrashed;
    bool bIsNewlyCrashed;
    bool bIsNewlyStarted;
    
    
    
private:
    
    
    //inputs
    ofThreadChannel<NewFrame> newFrame_IN;
    
    
    //outputs
    ofThreadChannel<ofPixels> newPix_OUT;

    
    //These are objects to be accessed --ONLY--
    //from within thread.
//    ofPixels pix_thread;
    
    NewFrame nf;
    
    void threadedFunction();
    
    
    
};