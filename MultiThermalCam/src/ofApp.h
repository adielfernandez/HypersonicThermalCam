#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxOsc.h"
#include "ofxThermalClient.h"
#include "Zone.hpp"

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
    
    //cam IDs
    const int cam1Id = 437404672; //REAL
    
    //Faked for testing:
    const int cam2Id = 437404673;
    const int cam3Id = 437404674;
    
    float lastFrameRate;
    float camFrameRate;
    double lastFrameTime;
    
    const int camWidth = 206;
    const int camHeight = 156;
    
    //Content layout
    
    //0 = main view
    //1 - stitching mode view
    int viewMode;
    int currentView;
    const int numViews = 2;
    
    float leftMargin;
    float topmargin;
    float gutter;
    
    ofVec2f slot1;
    ofVec2f slot2;
    ofVec2f slot3;
    ofVec2f slot4;
    ofVec2f slot5;
    ofVec2f primarySlot;
    float primarySlotScale;
    
    //adjusted mouse position within
    //primary slot and scaled down to camera dims
    ofVec2f adjustedMouse;
    
    //-----pixel objects-----
    
    //these hold the persistent
    //images from individual camera
    ofPixels rawGrayPix1;
    ofPixels rawGrayPix2;
    ofPixels rawGrayPix3;
    
    //raw pix are blended into
    ofPixels masterPix;
    int masterWidth, masterHeight;
    
    //then master pix is fed into the
    //following objects
    ofPixels processedPix;
    ofPixels threshPix;
    ofPixels backgroundPix;
    ofPixels foregroundPix;
    
    
    int pixelAverage;
    float stdDev, avgVariance;
    bool frameBlackOut;
    
    vector<int> pixelBins;
    vector<int> varianceBins;
    

    
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
    
    double lastOSCSendTime;
    
    //-----Detection zones-----
    //0 = danger zone
    //1, 2, 3 progressivel larger zones
    const int numZones = 4;
    vector<Zone> zones;
    int activeZone;
    
    //a bool for each of the detection
    //zone points for manipulating with mouse
    //4 zones, 4 points each
    vector<bool> mouseLockPoints;
    int zonePtRad;
    
    
    //-----GUI SETUP-----
    void setupGui();
    void drawGui(int x, int y);
    void loadSettings();
    void saveSettings();
    
    void applyGuiValsToZones();
    
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
    ofxToggle stdDevBlackOutToggle;
    ofxIntSlider stdDevThreshSlider;
    
    ofxLabel contoursLabel;
    ofxIntSlider minBlobAreaSlider;
    ofxIntSlider maxBlobAreaSlider;
    ofxIntSlider persistenceSlider;
    ofxIntSlider maxDistanceSlider;
    ofxToggle drawContoursToggle;
    ofxToggle showInfoToggle;
    
    ofxLabel detectionLabel;
    ofxToggle showSecondGui;
    
    ofxLabel stitchingLabel;
    ofxButton trimMasterPixButton;
    
    
    ofxPanel gui2;
    string gui2Name;
    
    ofxLabel zonePointsLabel;
    ofxVec2Slider gui2Pos;
    ofxVec2Slider dangerPt0;
    ofxVec2Slider dangerPt1;
    ofxVec2Slider dangerPt2;
    ofxVec2Slider dangerPt3;
    ofxVec2Slider active1Pt0;
    ofxVec2Slider active1Pt1;
    ofxVec2Slider active1Pt2;
    ofxVec2Slider active1Pt3;
    ofxVec2Slider active2Pt0;
    ofxVec2Slider active2Pt1;
    ofxVec2Slider active2Pt2;
    ofxVec2Slider active2Pt3;
    ofxVec2Slider active3Pt0;
    ofxVec2Slider active3Pt1;
    ofxVec2Slider active3Pt2;
    ofxVec2Slider active3Pt3;
    
    ofxLabel stitchingPointsLabel;
    ofxVec2Slider cam1Pos;
    ofxVec2Slider cam2Pos;
    ofxVec2Slider cam3Pos;
    

};
