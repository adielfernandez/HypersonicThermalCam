//
//  Zone.cpp
//  thermalCam
//
//  Created by Adiel Fernandez on 3/29/17.
//
//

#include "Zone.hpp"


Zone::Zone(){
    
    
}

void Zone::setup(int num){
    
    //0 = danger
    //1, 2, 3 = progressively further out
    zoneNum = num;
    mouseLockPoints.assign(4, false);
    
    ptRad = 8;
    
    path.setFilled(false);
    path.setStrokeWidth(2);
    
    if(zoneNum == 0){
        col.set(255, 0, 0);
    } else if (zoneNum == 1){
        col.set(255, 100, 0);
    } else if (zoneNum == 2){
        col.set(255, 200, 0);
    } else {
        col.set(0, 255, 0);
    }
    
    path.setStrokeColor(col);
    
    bNeedsUpdate = false;
    
}

void Zone::setGuiRefs(ofxVec2Slider *_p0, ofxVec2Slider *_p1, ofxVec2Slider *_p2, ofxVec2Slider *_p3){
    
    p0 = _p0;
    p1 = _p1;
    p2 = _p2;
    p3 = _p3;
    
}

void Zone::setPoints(ofVec2f p0, ofVec2f p1, ofVec2f p2, ofVec2f p3){
    
    points.clear();
    points = { p0, p1, p2, p3 };
    bNeedsUpdate = true;
    
}

void Zone::setPathFromVector(){
    
    path.clear();
    
    path.moveTo(points[0]);
    path.lineTo(points[1]);
    path.lineTo(points[2]);
    path.lineTo(points[3]);
    path.lineTo(points[0]);
    path.close();
    
}

void Zone::update(){
    
    if( bNeedsUpdate ){
        setPathFromVector();
    }
    
}


void Zone::draw(float scaleUp){
    path.draw();
    
    //draw mouse Points too
    ofPushStyle();
    ofSetColor(col);
    ofNoFill();
    ofSetLineWidth(2);
    for(int i = 0; i < points.size(); i++){
        //divide rad by scale so it looks the right size when scaled up
        ofDrawCircle(points[i], ptRad/scaleUp);
        ofDrawBitmapString(ofToString(i), points[i].x + 3, points[i].y - 3);
        
    }
    ofPopStyle();
}


void Zone::releasePoints(){
    
    std::fill( mouseLockPoints.begin(), mouseLockPoints.end(), false );
    
}


//sets the point to the mouse value
//does nothing if no points are locked on mouse
void Zone::setClickedPoint(int x, int y){
    
    for( int i = 0; i < mouseLockPoints.size(); i++){

        if( mouseLockPoints[i] ){
            points[i].set(x, y);
            
            //assign the point to the appropriate gui value
            if(i == 0){
                (*p0) = ofVec2f(x, y);
            } else if (i == 1){
                (*p1) = ofVec2f(x, y);
            } else if (i == 2){
                (*p2) = ofVec2f(x, y);
            } else {
                (*p3) = ofVec2f(x, y);
            }
            
            break;
        }
        
    }
    
    update();
    
}


bool Zone::checkForClicks( int x, int y ){
    
    releasePoints();
    
    bool foundClicked = false;
    
    for(int i = 0; i < mouseLockPoints.size(); i++){
        
        float d = ofDist(x, y, points[i].x, points[i].y );
        
        if( d < ptRad ){
            mouseLockPoints[i] = true;
            foundClicked = true;
            break;
        }
    }
    
    return foundClicked;
    
}





