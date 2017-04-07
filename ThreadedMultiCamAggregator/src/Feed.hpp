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

#pragma once


class Feed{
    
public:
    Feed();
    
    
    void setup(int _id, int w, int h);
    void newFrame(ofPixels raw, ofPixels gray);
    void adjustContrast( ofPixels *pix, float exp, float phase);
    
    void setValsFromGui(float exp, float phase, float stdDev);
    
    ofPixels getOutputPix();
    
    void draw(int x, int y);
    
    int camID;
    
    
    //gui values
    ofxFloatSlider *contrastExp;
    ofxFloatSlider *contrastPhase;
    ofxIntSlider *stdDevThresh;
    ofxToggle *stdDevToggle;
    
    
    int camWidth, camHeight;
    
    ofPixels rawPix;
    ofPixels grayPix;
    ofPixels blackPix;
    ofImage img;
    
    float camFrameRate, lastFrameRate;
    double lastFrameTime;
    
    PixelStatistics pixelStats;
    bool bDropThisFrame;
    
};