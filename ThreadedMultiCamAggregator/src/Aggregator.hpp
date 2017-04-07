//
//  Aggregator.hpp
//  ThreadedMultiCamAggregator
//
//  Created by Adiel Fernandez on 4/7/17.
//
//

#ifndef Aggregator_hpp
#define Aggregator_hpp

#include <stdio.h>

#endif /* Aggregator_hpp */

#include "ofMain.h"
#include "ofxCv.h"

#pragma once


class Aggregator: public ofThread{
    
public:
    
    Aggregator();
    void setup(int singleW, int singleH);
    void update();
    void analyze(ofPixels *pix, vector<int> settings);
    
    int camWidth;
    int camHeight;
    
    unsigned long long lastFrameTime;
    float lastFrameRate; //for smoothing
    float camFrameRate;
    
    //pointers to pixel objects in ofApp
    //we will update them when we get
    //new data from thread
    ofPixels processedPix;
    ofPixels threshPix;
    ofPixels backgroundPix;
    ofPixels foregroundPix;
    
private:
    
    
    unsigned long long frameNum;
    
    //thread management
    void closeAllChannels();
    void emptyAllChannels();
    bool isThreadCrashed;
    
    //into thread
    ofThreadChannel<vector<int>> settingsIn;
    ofThreadChannel<ofPixels> rawPixIn;
    
    //Thread output
    ofThreadChannel<ofPixels> threshPixOut;
    ofThreadChannel<ofPixels> backgroundPixOut;
    ofThreadChannel<ofPixels> foregroundPixOut;
    ofThreadChannel<ofxCv::ContourFinder> contoursOut;
    
    //this member exists outside of the thread
    //so contours maintain their permanence (IDs)
    //but DO NOT use this anywhere outside of thread
    ofxCv::ContourFinder contours_thread;
    ofxCv::RunningBackground background;
    
    //for restarting the background learning
    bool needsAutoReset;
    
    //for restarting the thread
    unsigned long long lastRestartTime;
    bool firstAfterCrash;
    bool firstRestart;
    
    
    void threadedFunction();
    
};

