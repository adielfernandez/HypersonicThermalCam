/*
 * ofxThermalClient.mm
 */

#include "ofxThermalClient.h"
#include "ofxThermalDelegate.h"

#import <VVSeekThermalUSB/VVSeekThermalUSB.h>
//#import <Cocoa/Cocoa.h>

ofxThermalClient::ofxThermalClient() //: mDevice(nil)
{
    
}

void ofxThermalClient::setup()
{
    camDelegate = [[ofxThermalDelegate alloc] init];
    [(ofxThermalDelegate*)camDelegate setup];
//    NSLog(@"Calling this function from OF!");
}

void ofxThermalClient::checkForNewFrame()
{
    if (((ofxThermalDelegate*) camDelegate).hasNewFrame)
    {
        
        NewFrameData nf;
        
        nf.ID = getDeviceLocation();
        nf.pix.setFromPixels(getPixels(), 206, 156, OF_PIXELS_RGBA);
        
        ofNotifyEvent(newFrameEvt, nf, this);
        
        ((ofxThermalDelegate*) camDelegate).hasNewFrame = false;
       
    }
    
    
}
unsigned char* ofxThermalClient::getPixels(){
    return ((ofxThermalDelegate*) camDelegate).frameData;
}

int ofxThermalClient::getDeviceLocation(){
    return ((ofxThermalDelegate*) camDelegate).deviceLocation;
}




