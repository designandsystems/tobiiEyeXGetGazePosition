#pragma once

#include "ofMain.h"
#include "EyeX.h"
#include "EyeXHost.h"



// window messages used for notifications from the EyeXHost.
#define WM_EYEX_HOST_STATUS_CHANGED     WM_USER + 0
#define WM_REGION_GOT_ACTIVATION_FOCUS  WM_USER + 1
#define WM_REGION_ACTIVATED             WM_USER + 2

class ofApp : public ofBaseApp{
	public:

		//EyeXHost();
		//virtual ~EyeXHost();

		EyeXHost eyex;

		void setup();
		void update();
		void draw();
		
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);


		
};
