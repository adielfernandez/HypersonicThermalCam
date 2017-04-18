#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxCv.h"
#include "ofxGui.h"
#include "ofxOsc.h"
#include "ofxThermalClient.h"
#include "Zone.hpp"
#include "PixelStatistics.hpp"
#include "Feed.hpp"
#include "Aggregator.hpp"

#define NUM_CAMS 6


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
    
    const int camWidth = 206;
    const int camHeight = 156;

    //cam IDs - equate to USB HUB PORTS
    const int testID = 336592896;
    int camToFeed = 0;

    vector<Feed> feeds;
    Aggregator aggregator;
    
    
    
    
    list<ofxThermalClient::NewFrameData> frameQueue;
    void addNewFrameToQueue( ofxThermalClient::NewFrameData &nf );
    
    
    //Content layout
    
    //0 = Cams 0-1
    //1 = Cams 2-3
    //2 = Cams 4-5
    //3 = stitching mode view
    //4 = Masking view
    //5 = Pipeline
    //6 = zones view
    //7 = "Headless" view
    int viewMode;
    int currentView;
    const int numViews = 8;
    
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
    
    //raw pix are blended into
    ofPixels masterPix;
    int masterWidth, masterHeight;
    int oldMasterWidth, oldMasterHeight;
    
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
    
    ofxLabel OSCLabel;
    ofxToggle sendOSCToggle;
    ofxFloatSlider waitBeforeOSCSlider;
    ofxFloatSlider maxOSCSendRate;
    
    ofxLabel detectionLabel;
    ofxToggle showSecondGui;
    
    
    //Mask stuff
    ofxPanel maskingGui;
    string maskGuiName;
    ofxVec2Slider maskGuiPos;
    ofxToggle useMask;
    ofxToggle drawOrErase;
    ofxButton clearMask;
    ofxButton saveMask;
    ofxButton loadMask;
    
    string maskFileName;
    ofColor maskCol;
    ofPixels maskPix;
    ofImage maskImg;
    
    ofVec2f maskScreenPos;
    ofVec2f maskMousePos;
    
    int maskToolSize;
    bool maskPressed;
    bool bMouseInsideMask;
    bool cursorShowing, lastFrameCursorShowing;
    
    
//    ofVec2f maskBoundStart, maskBoundEnd;
    
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
    
    ofxVec2Slider camPositions[NUM_CAMS];
    ofxIntSlider camRotations[NUM_CAMS];
    ofxToggle camMirrorToggles[NUM_CAMS];
    


};
