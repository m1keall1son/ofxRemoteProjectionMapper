//
//  ofServerApp.hpp
//  RemoteProjectionMapper
//
//  Created by Michael Allison on 5/1/18.
//

#pragma once

#include "ofMain.h"
#include "ofxRemoteUIServer.h"
#include "ofxRemoteProjectionMapper.h"

class ofApp : public ofBaseApp{
    
public:
    void setup();
    void draw();
    
    ofxRemoteProjectionMapper mProjectionMapper;
    ofImage mImage;
    
};

