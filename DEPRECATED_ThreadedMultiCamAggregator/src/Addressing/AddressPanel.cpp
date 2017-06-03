//
//  AddressPanel.cpp
//  ThreadedMultiCamAggregator
//
//  Created by Adiel Fernandez on 4/21/17.
//
//

#include "AddressPanel.hpp"


AddressPanel::AddressPanel(){
    
    
}

void AddressPanel::setup(vector<int> *addr){
    
    //get reference to addres vector
    addresses = addr;
    numAddresses = addresses -> size();
    
    
    addrWidth = 350;
    addrHeight = 30;
    float margin = 5;
    
    float overallHeight = addrHeight * numAddresses + margin * (numAddresses - 1) ;
    
    arrowWidth = 80;
    arrowHeight = (overallHeight - margin)/2.0f;
    
    
    font.load("fonts/Aller_Rg.ttf", 14);

    
    
    buttons.clear();
    bPositions.clear();
    
    
    
    
    //setup the button objects
    //first 6 are address buttons, last two are up/down arrows
    for(int i = 0; i < numAddresses; i++){
        
        Button b;
        b.setup( &font );
            
        b.bIsArrow = false;
        b.bIsHidden = false;
        
        b.width = addrWidth;
        b.height = addrHeight;
        
        bPositions.push_back( ofVec2f(0, i * (addrHeight + margin)) );

        
        buttons.push_back(b);
        
    }
    
    //two arrow buttons
    for(int i = 0; i < 2; i++){
    
        Button b;
        b.setup( &font );
        
        //setup arrow buttons
        b.bIsArrow = true;
        b.bArrowType = i == 0 ? true : false;
        
        b.width = arrowWidth;
        b.height = arrowHeight;
        
        b.bIsHidden = true;
        
        //center the buttons vertically
        if( i == 0 ){
            bPositions.push_back( ofVec2f(addrWidth + margin, 0) );
        } else {
            bPositions.push_back( ofVec2f(addrWidth + margin, overallHeight/2.0f + margin/2.0f) );
        }
        
        buttons.push_back(b);

    }
    
    
    
    selectedIndex = -1;
    
    //change the enumeration of buttons as a convenience
    UP_BUTTON = numAddresses;
    DOWN_BUTTON = numAddresses + 1;
    
    
    
}

void AddressPanel::update(){
    
    //hide arrows if there is no selection
    //or if we're at the top or bottom
    if( selectedIndex == -1 ){
        buttons[UP_BUTTON].bIsHidden = true;
        buttons[DOWN_BUTTON].bIsHidden = true;
    } else {

        //unhide them both on first then hide later
        //based on selection
        buttons[UP_BUTTON].bIsHidden = false;
        buttons[DOWN_BUTTON].bIsHidden = false;
        
        //hide UP arrow if selection is 0
        if( selectedIndex == 0 ){
            buttons[UP_BUTTON].bIsHidden = true;
            
            //hide DOWN arrow if selection is 5
        } else if( selectedIndex == numAddresses - 1 ){
            buttons[DOWN_BUTTON].bIsHidden = true;
        }
        
        
    }
    
    
    //check button states and swap
    
    
    //UP arrow
    if( buttons[UP_BUTTON].bIsSelected ){
        //swap out the buttons based on the selection
        iter_swap(buttons.begin() + selectedIndex, buttons.begin() + (selectedIndex - 1) );

        //and swap the addresses
        iter_swap((*addresses).begin() + selectedIndex, (*addresses).begin() + (selectedIndex - 1) );
        
        buttons[UP_BUTTON].bIsSelected = false;
        
        selectedIndex -= 1;
    }
    
    //DOWN arrow
    if( buttons[DOWN_BUTTON].bIsSelected ){
        //swap out the buttons based on the selection
        iter_swap(buttons.begin() + selectedIndex, buttons.begin() + (selectedIndex + 1) );
        
        iter_swap((*addresses).begin() + selectedIndex, (*addresses).begin() + (selectedIndex + 1) );
        
        buttons[DOWN_BUTTON].bIsSelected = false;
        
        selectedIndex += 1;
        
        
    }
    
}

void AddressPanel::draw(int x, int y){
    
    drawPosition.set(x,y);
    
    //go through address buttons
    for(int i = 0; i < buttons.size(); i++){

        //set the button text according to the addresses
        string text = "Cam " + ofToString(i) + ": " + ofToString( (*addresses)[i] );
        
        buttons[i].text = text;
        
        buttons[i].draw( bPositions[i].x + drawPosition.x, bPositions[i].y + drawPosition.y );

    }
    
}

void AddressPanel::checkForMouseClicks(int x, int y){
    
    //shift the x and y to account for the drawing position on screen
    x -= drawPosition.x;
    y -= drawPosition.y;
    
    //go through the address buttons first
    for(int i = 0; i < numAddresses; i++){
        
        if( x > bPositions[i].x && x < bPositions[i].x + buttons[i].width && y > bPositions[i].y && y < bPositions[i].y + buttons[i].height ){
            
            //turn off all the other address buttons
            for(int j = 0; j < buttons.size(); j++){

                //the one clicked
                if( j == i ){
            
                    //if it's already selected, un select
                    if( buttons[i].bIsSelected ){
                        
                        buttons[i].bIsSelected = false;
                        selectedIndex = -1;
                        
                    } else {
                    
                        buttons[i].bIsSelected = true;
                        selectedIndex = i;
                    }
                    
                    
                    
                } else {
                    
                    buttons[j].bIsSelected = false;
                }
                
            }
         
            //we found a button, no need to check the others
            break;
        }
        
    }
    
    //if there's an active selection check the other buttons
    if( selectedIndex != -1 ){
        
        //no go through the arrow buttons
        for(int i = numAddresses; i < buttons.size(); i++){
            
            //only check if not hidden
            if( buttons[i].bIsHidden == false ){

                if( x > bPositions[i].x && x < bPositions[i].x + buttons[i].width && y > bPositions[i].y && y < bPositions[i].y + buttons[i].height ){
                    
                    //set to true, we'll turn it off later
                    buttons[i].bIsSelected = true;
                    
                }
                
            }
            
        }

    }
    
    
    
}




