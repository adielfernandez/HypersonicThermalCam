#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxOsc.h"
#include "ofxThermalClient.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
    ofxThermalClient thermal;
    float lastFrameRate;
    float camFrameRate;
    double lastFrameTime;
    
    
    const int camWidth = 206;
    const int camHeight = 156;
    
    
    
    //pixel objects
    ofxCvColorImage rawImg;
    ofxCvGrayscaleImage grayImg;
    
    ofPixels processedPix;
    ofPixels threshPix;
    ofPixels backgroundPix;
    
    int processedPixelAvg;
    bool frameBlackOut;
    
    ofxCv::ContourFinder contours;
    ofxCv::RunningBackground background;
    bool bNeedBGReset;
    
    //for drawing the saving/loading
    //feedback on screen
    double lastSaveTime, lastLoadTime;
    void drawSaveLoadBox();

    //-----OSC SETUP-----
    ofxOscSender osc;
    string oscIP;
    int oscPort;
    
    double lastActiveTriggerTime, lastDangerTriggerTime;
    
    //-----Detection zones-----
    //0 = danger zone
    //1, 2, 3 progressivel larger zones
    vector<ofRectangle> detectionZones;
    vector<bool> bInZone;
    
    
    //-----GUI SETUP-----
    void setupGui();
    void drawGui(int x, int y);
    void loadSettings();
    void saveSettings();
    
    ofxPanel gui;
    string guiName;
    
    ofxLabel imageAdjustLabel;
    ofxFloatSlider contrastExpSlider;
    ofxFloatSlider contrastPhaseSlider;
    ofxIntSlider blurAmountSlider;
    ofxIntSlider thresholdSlider;
    ofxIntSlider numErosionsSlider;
    ofxIntSlider numDilationsSlider;

    
    ofxLabel bgDiffLabel;
    ofxIntSlider learningTime;
    ofxButton resetBGButton;
    ofxToggle useBgDiff;
    ofxToggle useThreshold;
    ofxToggle averagePixelsToggle;
    ofxIntSlider blackOutThreshSlider;
    
    ofxLabel contoursLabel;
    ofxIntSlider minBlobAreaSlider;
    ofxIntSlider maxBlobAreaSlider;
    ofxIntSlider persistenceSlider;
    ofxIntSlider maxDistanceSlider;
    ofxToggle drawContoursToggle;
    ofxToggle showInfoToggle;
    
    ofxLabel detectionLabel;
    ofxVec2Slider dangerRegionStart;
    ofxVec2Slider dangerRegionEnd;
    ofxVec2Slider activeRegion1Start;
    ofxVec2Slider activeRegion1End;
    ofxVec2Slider activeRegion2Start;
    ofxVec2Slider activeRegion2End;
    ofxVec2Slider activeRegion3Start;
    ofxVec2Slider activeRegion3End;
    
//    ofxLabel maskingLabel;
//    ofxToggle useMask;
//    ofxToggle drawOrErase;
//    ofxButton clearMask;
//    ofxButton saveMask;
//    ofxButton loadMask;
    
    

};
