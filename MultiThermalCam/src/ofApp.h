#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxOsc.h"
#include "ofxThermalClient.h"
#include "Zone.hpp"
#include "PixelStatistics.hpp"


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
    
    ofTrueTypeFont titleFont;
    ofTrueTypeFont smallerFont;
    ofVec2f titlePos;
    
    //cam IDs - equate to USB HUB PORTS
    //Mac Mini IDs
    const int cam1Id = 343150592; //PORT1
    const int cam2Id = 343154688; //PORT2
    const int cam3Id = 343159872; //PORT3
    
    const int testID = 336592896;
    
//    const int cam1Id = 336658432; //PORT1
//    const int cam2Id = 336723968; //PORT2
//    const int cam3Id = 336789504; //PORT3
    
    
    void addNewFrameToQueue( ofxThermalClient::NewFrameData &nf );
    
    
    list<ofxThermalClient::NewFrameData> frameQueue;
    
    
    //frame rate data
    //for each camera
    
    vector<float> lastFrameRates;
    vector<float> camFrameRates;
    vector<double> lastFrameTimes;
    
    const int camWidth = 206;
    const int camHeight = 156;
    
    //Content layout
    
    //0 = main view
    //1 - stitching mode view
    int viewMode;
    int currentView;
    const int numViews = 3;
    
    float leftMargin;
    float topMargin;
    float gutter;
    
    ofVec2f slot1;
    ofVec2f slot2;
    ofVec2f slot3;
    ofVec2f slot4;
    ofVec2f slot5;
    ofVec2f slot6;

    ofVec2f detectionDisplayPos;

    float compositeDisplayScale;
    float pipelineDisplayScale;
    
    //adjusted mouse position within
    //primary slot and scaled down to camera dims
    ofVec2f adjustedMouse;
    
    //-----pixel objects-----
    
    //these hold the persistent
    //images from individual camera
    ofPixels rawPix1;
    ofPixels rawPix2;
    ofPixels rawPix3;
    
    ofPixels grayPix1;
    ofPixels grayPix2;
    ofPixels grayPix3;
    
    //raw pix are blended into
    ofPixels masterPix;
    int masterWidth, masterHeight;
    
    //then master pix is fed into the
    //following objects
    ofPixels processedPix;
    ofPixels threshPix;
    ofPixels backgroundPix;
    ofPixels foregroundPix;
    
    //a totally black frame for convenience
    ofPixels blackFrame;
    ofPixels blackFrameRot90;
    
    void adjustContrast(ofPixels *pix, float exp, float phase);
    
    vector<PixelStatistics> pixStats;
    
    
    ofxCv::ContourFinder contours;
    ofxCv::RunningBackground background;
    int backgroundWidth, backgroundHeight;
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
    
    
    ofxPanel stitchingGui;
    string stitchingGuiName;
    
    ofxLabel stitchingLabel;
    ofxVec2Slider stitchingGuiPos;
    ofxButton trimMasterPixButton;
    ofxVec2Slider cam1Pos;
    ofxVec2Slider cam2Pos;
    ofxVec2Slider cam3Pos;
    ofxIntSlider cam1Rotate90Slider;
    ofxIntSlider cam2Rotate90Slider;
    ofxIntSlider cam3Rotate90Slider;

};
