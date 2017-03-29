//
//  ofxThermalDelegate.m
//  thermalCam
//

#import <Foundation/Foundation.h>
#import <VVSeekThermalUSB/VVSeekThermalUSB.h>


@interface ofxThermalDelegate : NSObject <SeekThermalDeviceDelegate>	{
    SeekThermalDevice		*device;
    unsigned char *_frameData;
    BOOL _hasNewFrame;
}

-(void) setup;
@property (readwrite) unsigned char *frameData;
@property (readwrite) BOOL hasNewFrame;

@end
