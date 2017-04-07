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

void Feed::setup(int _id, int w, int h){
    
    camID = _id;
    camWidth = w;
    camHeight = h;
    
    rawPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    grayPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    
    
    blackPix.allocate(camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    blackPix.setColor(0);
    
    pixelStats.setup();
    bDropThisFrame = false;
    
}



void Feed::newFrame( ofPixels raw, ofPixels gray ){
    
    rawPix = raw;
    grayPix = gray;
    
    adjustContrast( &grayPix , (*contrastExp), (*contrastPhase) );
    
    
    //store some framerate data
    float thisFrameRate = 1.0/( (ofGetElapsedTimef() - lastFrameTime) );
    
    //log frame rates for each camera and average with the
    //last recorded frame rate to smooth a little
    camFrameRate = (thisFrameRate + lastFrameRate)/2;
    lastFrameRate = thisFrameRate;
    lastFrameTime = ofGetElapsedTimef();
    
    if( *stdDevToggle ){
        
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

ofPixels Feed::getOutputPix(){
    
    if( bDropThisFrame ){
        return blackPix;
    } else {
        return grayPix;
    }
    
}


void Feed::draw(int x, int y){
    
    ofPushStyle();
    
    ofSetColor(255);
    ofSetLineWidth(1);
    ofDrawBitmapString("FR: "  + ofToString(camFrameRate), x, y-5);
    
    img.setFromPixels(rawPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    img.draw(x, y);
    
    img.setFromPixels(grayPix.getData(), camWidth, camHeight, OF_IMAGE_GRAYSCALE);
    img.draw(x + camWidth, y);
    
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
