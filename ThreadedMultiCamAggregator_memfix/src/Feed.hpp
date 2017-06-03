//
//  Feed.hpp
//  ThreadedMultiCamAggregator
//
//  Created by Adiel Fernandez on 4/7/17.
//
//

#ifndef Feed_hpp
#define Feed_hpp

#include <stdio.h>

#endif /* Feed_hpp */

#include "ofMain.h"
#include "PixelStatistics.hpp"
#include "ofxGui.h"
#include "PreCompositeThreadCV.hpp"


#pragma once


class Feed{
    
public:
    Feed();
    
    //make a dummy copy constructor
    Feed(const Feed &f);
    
    void setup(int num, int _id, int w, int h);
    void newFrame(ofPixels &raw);
    void update();
    void adjustContrast( ofPixels *pix, float exp, float phase);
    void setValsFromGui(float exp, float phase, float stdDev);
    
    void drawRaw(int x, int y);
    void drawRawAndProcessed(int x, int y);
    ofPixels getOutputPix();
    
    void resetAllPixels();
    
    PreCompositeThreadCV threadedCV;
    
    
    int camID;
    int camNum;
    
    
    //gui values
    ofxIntSlider *blurAmt;
    ofxFloatSlider *contrastExp;
    ofxFloatSlider *contrastPhase;
    
    ofxIntSlider *stdDevThresh;
    ofxIntSlider *avgPixThresh;
    ofxToggle *stdDevToggle;
    
    
    int camWidth, camHeight;
    
    ofImage rawImg;
//    ofPixels rawPix;
    ofPixels grayPix;
    ofPixels blackPix;
    ofImage img;
    
    float camFrameRate, lastFrameRate;
    float lastFrameTime, timeSinceLastFrame;
    
    PixelStatistics pixelStats;
    bool bDropThisFrame;
    
};