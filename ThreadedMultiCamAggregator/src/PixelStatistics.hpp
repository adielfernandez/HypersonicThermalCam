//
//  PixelStatistics.hpp
//  MultiThermalCam
//
//  Created by Adiel Fernandez on 3/31/17.
//
//

#ifndef PixelStatistics_hpp
#define PixelStatistics_hpp

#include <stdio.h>

#endif /* PixelStatistics_hpp */


#include "ofMain.h"

#pragma once

class PixelStatistics{
    
public:
    
    PixelStatistics();
    
    void setup();
    void analyze(const ofPixels * const pix);
    void setStdDevThresh(float t);
    void drawDistribution(int x, int y, int w, int h);
    
    int camNum;
    
    int pixelAverage;
    float stdDev, avgVariance;
    
    float threshold;
    
    bool bDataIsBad;
    
    vector<int> pixelBins;
    vector<int> varianceBins;
    
    
    
};