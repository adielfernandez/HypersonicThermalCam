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
    NSLog(@"Calling this function from OF!");
}

void ofxThermalClient::checkForNewFrame()
{
    if (((ofxThermalDelegate*) camDelegate).hasNewFrame)
    {
       // NSLog(@"new frame seen!");
        receivedNewFrame = true;
    }
    
    
}
unsigned char* ofxThermalClient::getPixels(){
    //Note that we've caught the latest frame
    //NOTE: These double booleans are probably unecessary and could be cleaned up
    receivedNewFrame = false;
    ((ofxThermalDelegate*) camDelegate).hasNewFrame=false;
    
    return ((ofxThermalDelegate*) camDelegate).frameData;
}

int ofxThermalClient::getDeviceLocation(){
    
    return ((ofxThermalDelegate*) camDelegate).deviceLocation;
    
}




