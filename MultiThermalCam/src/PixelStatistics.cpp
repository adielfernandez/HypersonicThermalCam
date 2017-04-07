//
//  PixelStatistics.cpp
//  MultiThermalCam
//
//  Created by Adiel Fernandez on 3/31/17.
//
//

#include "PixelStatistics.hpp"


PixelStatistics::PixelStatistics(){
    
    
}

void PixelStatistics::setup(int num){
    
    camNum = num;
    
    bDataIsBad = false;
    
    //make a bin for each possible pixel value
    //then we'll draw a bar chart later to visualize the
    //statistical break down
    pixelBins.assign(256, 0);
    
    varianceBins.assign(256, 0);
    
    
}

void PixelStatistics::setStdDevThresh(float t){
    threshold = t;
}

void PixelStatistics::analyze(const ofPixels * const pix){
    
    bDataIsBad = false;
    
    //make a bin for each possible pixel value
    //then we'll draw a bar chart later to visualize the
    //statistical break down
    pixelBins.assign(256, 0);
    
    varianceBins.assign(256, 0);
    
    pixelAverage = 0;
    stdDev = 0;
    int numSamples = 0;
    
    //fill the pixel val vector with 0's
    std::fill( pixelBins.begin(), pixelBins.end(), 0 );
    std::fill( varianceBins.begin(), varianceBins.end(), 0 );
    
    for(int i = 0; i < pix -> getWidth() * pix -> getHeight(); i++){
        
        pixelAverage += (*pix)[i];
        numSamples++;
        
        //add one to each bin depending on the pixel value
        pixelBins[ (int)(*pix)[i] ] += 1;
        
    }
    
    pixelAverage /= numSamples;
    
    
    //we need the average number of pixels per bin to get the variance
    int avgPixPerBin = 0;
    
    for(int i = 0; i < pixelBins.size(); i++ ){
        avgPixPerBin += pixelBins[i];
    }
    
    avgPixPerBin /= pixelBins.size();
    
    
    //calculate the standard deviation of the pixelBins vector
    //first get the variance of each bin from the average
    // variance = square of the abs difference between value and average
    for(int i = 0; i < pixelBins.size(); i++){
        varianceBins[i] = pow( pixelBins[i] - avgPixPerBin, 2 );
    }
    
    //now go through again and find the average of all the variances
    avgVariance = 0;
    for(int i = 0; i < varianceBins.size(); i++){
        avgVariance += varianceBins[i];
    }
    
    avgVariance /= (float)varianceBins.size();
    
    //standard deviation = sqrt of variance average
    stdDev = sqrt(avgVariance);
    
    //should we block out the frame?
    bDataIsBad = (stdDev < threshold);
    
}

void PixelStatistics::drawDistribution(int x, int y, int w, int h){
    
    //put it in a pretty place
    ofPushStyle();
    ofPushMatrix();{
        
        ofVec2f graphOrigin(x, y);
        
        //graph draws from the origin so translate down so this
        //method draws at x and y
        ofTranslate(graphOrigin.x, graphOrigin.y + h);
        
        float maxYAxis = h;
        float maxXAxis = 256;   //256 = 1 px per bin
        
        float horizontalMult = w/maxXAxis;
        
        auto it = std::max_element( pixelBins.begin(), pixelBins.end() );
        int maxBinHeight = *it;
        
        //draw axis lines
        ofSetColor(0, 128, 255);
        ofSetLineWidth(2);
        ofDrawLine(0, 0, 0, -maxYAxis);
        ofDrawLine(0, 0, maxXAxis * horizontalMult, 0);
        
        ofSetLineWidth(2);
        ofSetColor(255);
        for( int i = 0; i < pixelBins.size(); i++){
            
            float v  = ofMap(pixelBins[i], 0, maxBinHeight, 0, maxYAxis);
            
            //move it a few pixels to the right so it doesn draw on the axis line
            float x = ( i * horizontalMult ) + 2;
            ofDrawLine( x, 0, x, -v );
            
        }
        
        //draw the average line
        ofSetColor(255, 0, 0);
        ofSetLineWidth(2);
        ofDrawLine(pixelAverage*horizontalMult + 3, 0, pixelAverage*horizontalMult + 3, -maxYAxis);
        ofDrawBitmapString("Avg: " + ofToString(pixelAverage), pixelAverage*horizontalMult + 5, -maxYAxis + 15);
        
        
        
    }ofPopStyle();
    ofPopMatrix();
    
    string stats = "Cam " + ofToString(camNum) + ": Std Dev: " + ofToString(stdDev);
    
    ofSetColor(255);
    ofDrawBitmapString(stats, x, y);
    
    if( bDataIsBad ){
        string s = "NOISY PROFILE DETECTED";
        ofSetColor(255, 0, 0);
        ofDrawBitmapString(s, x + 150, y + 45);
    }
    
}





