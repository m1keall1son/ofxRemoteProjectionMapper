#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
    ofGLFWWindowSettings winSettings;
    winSettings.numSamples = 8;
    winSettings.width = 1280;
    winSettings.height = 800;
    winSettings.windowMode = OF_WINDOW;
    //winSettings.multiMonitorFullScreen = true;
    winSettings.setGLVersion(3, 2);
    
    auto win = ofCreateWindow(winSettings);
    auto app = std::make_shared<ofApp>();
    
    ofRunApp(win, std::move(app));
    ofRunMainLoop();
    
}
