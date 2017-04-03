/*
 *  ofxThermal.h
 */

#include "ofMain.h"

class ofxThermalClient {
	public:
    
    ofxThermalClient();
    unsigned char* getPixels();
    void checkForNewFrame();
    
//    unsigned char* cameraPixels;
    void* camDelegate; //An objective C function that can use notifications properly
    bool receivedNewFrame;
    
    void setup();

    int getDeviceLocation();
    
    struct NewFrameEvt{
        ofPixels pix;
        int ID;
    };
    
    ofEvent<NewFrameEvt> newFrameEvt;
    

};
