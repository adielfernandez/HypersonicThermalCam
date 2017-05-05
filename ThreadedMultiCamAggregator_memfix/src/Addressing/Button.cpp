//
//  Button.cpp
//  ThreadedMultiCamAggregator
//
//  Created by Adiel Fernandez on 4/21/17.
//
//

#include "Button.hpp"


Button::Button(){
    
    
}

void Button::setup(ofTrueTypeFont *f){

    font = f;
    
    bIsSelected = false;
    
    baseCol.set(20);
    selectedCol.set(0, 128, 255);
    
}

void Button::update(){
 
    
    
    
    
}

void Button::draw(int x, int y){
    
    float transparency;
    
    if( bIsHidden == false ){
        transparency = 255;
    } else {
        transparency = 60;
    }
    
    
    ofPushMatrix();{
        ofTranslate(x, y);
        
        if( bIsSelected ){
            ofSetColor( selectedCol, transparency );
        } else {
            ofSetColor( baseCol, transparency );
        }
        
        ofDrawRectangle(0, 0, width, height);
        
        
        if( bIsArrow ){
            
            ofSetColor(255, transparency);
            
            //arrow points
            ofVec2f v0, v1, v2;
            float arrowHeight = height/3.0;
            float arrowWidth = width/2.0;
            
            //draw a triangle
            if( bArrowType ){
                
                //UP
                v0.set( width/2, height/2 - arrowHeight/2.0 ); //top point
                v1.set( width/2 - arrowWidth/2.0, height/2 + arrowHeight/2.0 ); //left point
                v2.set( width/2 + arrowWidth/2.0, height/2 + arrowHeight/2.0 ); //right point
                
            } else {
                
                //DOWN
                v0.set( width/2, height/2 + arrowHeight/2.0 ); //bottomw point
                v1.set( width/2 - arrowWidth/2.0, height/2 - arrowHeight/2.0 ); //left point
                v2.set( width/2 + arrowWidth/2.0, height/2 - arrowHeight/2.0 ); //right point
                
            }
            
            ofDrawTriangle(v0, v1, v2);
            
        } else {
            //X centered text
            //            float textMarginX = (width - font -> stringWidth(text))/2.0f;
            
            //left aligned
            float textMarginX = 20;
            float textMarginY = (height - font -> stringHeight(text))/2.0f;
            
            //centered text (draws from baseline)
            ofVec2f textPos( textMarginX, height - textMarginY );
            
            ofSetColor(255, transparency);
            font -> drawString(text, textPos.x, textPos.y);
            
        }
        
        
    }ofPopMatrix();
    
}



