//
//  Button.hpp
//  ThreadedMultiCamAggregator
//
//  Created by Adiel Fernandez on 4/21/17.
//
//

#ifndef Button_hpp
#define Button_hpp

#include <stdio.h>

#endif /* Button_hpp */


#include "ofMain.h"

#pragma once

class Button{
    
public:
    
    Button();
    void setup(ofTrueTypeFont *f);
    void update();
    void draw(int x, int y);
    
    bool bIsHidden;
    
    bool bIsSelected;
    
    bool bIsArrow;
    bool bArrowType;
    
    int buttonNum;
    string text;
    
    //styling and formatting
    ofColor baseCol, selectedCol;
    
    ofTrueTypeFont *font;
    
    float width, height;
    
    float lastClickTime;
    
    
};