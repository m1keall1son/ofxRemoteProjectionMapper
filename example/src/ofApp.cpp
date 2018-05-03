//
//  ofServerApp.cpp
//  RemoteProjectionMapper
//
//  Created by Michael Allison on 5/1/18.
//

#include "ofApp.h"

void ofApp::setup(){
    
    ofBackground(22);
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    ofEnableAlphaBlending();
    
    mProjectionMapper.setSaveLocation(ofToDataPath("mapping"));
    mProjectionMapper.init(); //call this before creating any textures
    
    mImage.load("test.png");
    mImage.getTexture().enableMipmap();

    mProjectionMapper.setContentSize(mImage.getWidth(), mImage.getHeight());
    
    /*
    create the warps using ofxRemoteUIClient by:
        1) setting the next warp name
        2) selecting one of the warp styles (perspective, bilinear, or perspective bilinear)
     
     or create them manually:
    
    auto myWarp = mProjectionMapper.createWarp<RemoteWarpBilinear>("test", WarpSettings()
                                                            .srcSize(mImage.getWidth(), mImage.getHeight())
                                                            .srcArea(ofRectangle(0,0, mImage.getWidth(),mImage.getHeight()))
                                                            .drawArea(ofRectangle(0,0, ofGetWidth(),ofGetHeight())
                                                            ));
     */
}

void ofApp::draw(){
    mProjectionMapper.drawWarps(mImage.getTexture());
}


