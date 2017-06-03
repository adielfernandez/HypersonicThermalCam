//
//  Zone.hpp
//  thermalCam
//
//  Created by Adiel Fernandez on 3/29/17.
//
//

#ifndef Zone_hpp
#define Zone_hpp

#include <stdio.h>

#endif /* Zone_hpp */


#include "ofMain.h"
#include "ofxGui.h"

#pragma once

class Zone{
    
public:
    
    Zone();
    
    void setup(int num);

    void setPoints(ofVec2f p0, ofVec2f p1, ofVec2f p2, ofVec2f p3);
    
    void setGuiRefs(ofxVec2Slider *_p0, ofxVec2Slider *_p1, ofxVec2Slider *_p2, ofxVec2Slider *_p3);
    
    void setPathFromVector();
    void update();
    void draw(float scaleUp);
    
    void releasePoints();
    bool checkForClicks( int x, int y );
    
    void setClickedPoint(int x, int y);
    
    int zoneNum;
    
    int ptRad;
    ofColor col;
    
    ofPath path;
    
    bool bNeedsUpdate;
    
    vector<ofVec2f> points;
    vector<bool> mouseLockPoints;
    
    ofxVec2Slider *p0;
    ofxVec2Slider *p1;
    ofxVec2Slider *p2;
    ofxVec2Slider *p3;
    
};