//
//  AddressPanel.hpp
//  ThreadedMultiCamAggregator
//
//  Created by Adiel Fernandez on 4/21/17.
//
//

#ifndef AddressPanel_hpp
#define AddressPanel_hpp

#include <stdio.h>

#endif /* AddressPanel_hpp */


#include "ofMain.h"
#include "Button.hpp"

#pragma once


class AddressPanel{
    
public:
    
    AddressPanel();
    
    void setup(vector<int> *addr);
    void update();
    void draw(int x, int y);
    
    void checkForMouseClicks(int x, int y);
    
    ofTrueTypeFont font;
    
    vector<int> *addresses;
    int numAddresses;
    
    //enum style variable
    //just to hold button numbers
    int UP_BUTTON = 0;
    int DOWN_BUTTON = 0;
        
    
    vector<Button> buttons;
    
    
    //these need to be stored separately
    //since buttons will be swapped within
    //vector but positions remain the same
    vector<ofVec2f> bPositions;
    
    int selectedIndex;
    
    float addrWidth, addrHeight;
    float arrowWidth, arrowHeight;
    float arrowButtonWidth;

    ofVec2f drawPosition;
    
    
};