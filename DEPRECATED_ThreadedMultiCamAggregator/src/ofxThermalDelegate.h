//
//  ofxThermalDelegate.m
//  thermalCam
//

#import <Foundation/Foundation.h>
#import <VVSeekThermalUSB/VVSeekThermalUSB.h>


@interface ofxThermalDelegate : NSObject <SeekThermalDeviceDelegate>	{
    NSArray *allDevices;
    SeekThermalDevice		*device;
    unsigned char *_frameData;
    NSBitmapImageRep	*rep;
    BOOL _hasNewFrame;
    int _deviceLocation;
}

-(void) setup;
-(int) getCameraCount;
@property (readwrite) unsigned char *frameData;
@property (readwrite) BOOL hasNewFrame;
@property (readwrite) int deviceLocation;


@end
