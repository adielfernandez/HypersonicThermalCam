#import "ofxThermalDelegate.h"


#define DEBUG_MODE

@implementation ofxThermalDelegate

@synthesize frameData=_frameData;
@synthesize hasNewFrame=_hasNewFrame;
@synthesize deviceLocation=_deviceLocation;

-(void) setup {
    device = nil;
    
    //Do some sort of initialization
    [SeekThermalDevice class];
    
    NSLog(@"Trying to find a camera");
    _frameData = nil;
    //	try to get a device right away
    allDevices = [SeekThermalDevice deviceArray];



//Handling one device:
//    if (allDevices!=nil && [allDevices count]>=1)	{
//        [self setDevice:[allDevices objectAtIndex:0]];
//    }
        //Multiple devices:
        for (int i=0; i<[allDevices count]; i++) {
            [self setDevice:[allDevices objectAtIndex:i]];
        }
    
    //	register to receive notifications that devices have been added or removed
    NSNotificationCenter		*ns = [NSNotificationCenter defaultCenter];
    [ns addObserver:self selector:@selector(newThermalDevice:) name:kSeekThermalDeviceAddedNotification object:nil];
    [ns addObserver:self selector:@selector(removedThermalDevice:) name:kSeekThermalDeviceRemovedNotification object:nil];

}

-(int) getCameraCount {
    return [allDevices count];
}

- (void) newThermalDevice:(NSNotification *)note	{
    NSLog(@"found a camera! %s",__func__);
    if ([self device]==nil)
        [self setDevice:[note object]];
}
- (void) removedThermalDevice:(NSNotification *)note	{
    NSLog(@"%s",__func__);
    if ([self device] == [note object])
        [self setDevice:nil];
}
- (SeekThermalDevice *) device	{
    return device;
}
- (void) setDevice:(SeekThermalDevice *)n	{
    device = n;
    if (device != nil)	{
        [device setDelegate:self];
        [device start];
    }
}

- (void) thermalCamera:(id)deviceWData hasNewFrameAvailable:(SeekThermalFrame *)newFrame	{
    
    _deviceLocation = ((SeekThermalDevice*)deviceWData).deviceLocation;
//    NSLog(@"frame from camera: %i",_deviceLocation);
    
    
    //	make a RGB bitmap rep, populate it with the contents of the frame
    NSSize				frameSize = [newFrame calibratedSize];
//    NSLog(@"framesize: width %f height %f", frameSize.width, frameSize.height);
    
    NSBitmapImageRep	*rep = [[NSBitmapImageRep alloc]
                                initWithBitmapDataPlanes:nil
                                pixelsWide:(long)frameSize.width
                                pixelsHigh:(long)frameSize.height
                                bitsPerSample:8
                                samplesPerPixel:4
                                hasAlpha:YES
                                isPlanar:NO
                                colorSpaceName:NSCalibratedRGBColorSpace
                                bitmapFormat:0
                                bytesPerRow:32 * (long)frameSize.width / 8
                                bitsPerPixel:32];
    
    if (rep == nil)	{
        NSLog(@"\t\terr: couldn't make bitmap rep, %s",__func__);
        return;
    }
    _frameData = [rep bitmapData];
    if (_frameData == nil)	{
        NSLog(@"\t\terr: rep bitmapData nil, %s",__func__);
        return;
    }
    //	the bitmap rep may have more bytes per row than the array of calibrated vals from the frame, so we need to know how many bytes per row are in the rep, and how many bytes per row we're actually writing to the rep
    NSInteger			repBytesPerRow = [rep bytesPerRow];
    NSInteger			minBytesPerRow = 8*4*(int)frameSize.width/8;
    
    
    uint8_t				*wPtr = _frameData;
    double				*rPtr = [newFrame calibratedVals];
    double				frameMin = [newFrame minCalibratedVal];
    double				frameMax = [newFrame maxCalibratedVal];
    
    for (int y=0; y<(int)frameSize.height; ++y)	{
        for (int x=0; x<(int)frameSize.width; ++x)	{
            *(wPtr+0) = (uint8_t)((*rPtr-frameMin)/(frameMax-frameMin)*255.);
            *(wPtr+1) = *(wPtr);
            *(wPtr+2) = *(wPtr);
            *(wPtr+3) = 255;
            wPtr += 4;
            ++rPtr;
        }
        wPtr += (repBytesPerRow - minBytesPerRow);
    }
    
    _hasNewFrame = true;
        
}

@end
