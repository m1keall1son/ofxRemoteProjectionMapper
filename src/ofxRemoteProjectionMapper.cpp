//
//  RemoteProjectionMapper.cpp
//  RemoteProjectionMapper
//
//  Created by Michael Allison on 5/1/18.
//

#include "ofxRemoteProjectionMapper.h"

ofxRemoteProjectionMapper::ofxRemoteProjectionMapper():
    saveLocation(ofToDataPath("mapping"))
{}

ofxRemoteProjectionMapper::~ofxRemoteProjectionMapper()
{
    saveWarps();
}

void ofxRemoteProjectionMapper::init(bool initRemoteUI, int port, float updateInterval, bool verbose)
{
    ofDisableArbTex();
    if(initRemoteUI){
        RUI_SETUP(port,updateInterval);
        if(verbose)
            RUI_GET_INSTANCE()->setVerbose(true);
    }
    
    ofAddListener(RUI_GET_OF_EVENT(), this, &ofxRemoteProjectionMapper::handleRemoteUpdate);
    
    RUI_NEW_COLOR();
    RUI_NEW_GROUP("Mapper");
    RUI_SHARE_PARAM_WCN("next warp name",nextWarpName);
    RUI_SHARE_PARAM_WCN("create persp warp",doCreatePerspectiveWarp);
    RUI_SHARE_PARAM_WCN("create bilinear warp",doCreateBilinearWarp);
    RUI_SHARE_PARAM_WCN("create perp bilinear warp",doCreatePerspectiveBilinearWarp);
    
    ofAddListener(ofEvents().mouseMoved, this, &ofxRemoteProjectionMapper::handleMouseMove);
    ofAddListener(ofEvents().mouseDragged, this, &ofxRemoteProjectionMapper::handleMouseDrag);
    ofAddListener(ofEvents().mousePressed, this, &ofxRemoteProjectionMapper::handleMouseDown);
    ofAddListener(ofEvents().mouseReleased, this, &ofxRemoteProjectionMapper::handleMouseUp);
    ofAddListener(ofEvents().keyPressed, this, &ofxRemoteProjectionMapper::handleKeyPress);
    ofAddListener(ofEvents().keyReleased, this, &ofxRemoteProjectionMapper::handleKeyReleased);

    loadWarps();
}

void ofxRemoteProjectionMapper::handleKeyPress(ofKeyEventArgs& args)
{
    if(args.key == OF_KEY_CONTROL){
        selectingMultiple = !selectingMultiple;
        if(!selectingMultiple){
            selectionAreaSet = false;
            for(auto & warp : mappings){
                warp->deselectAllControlPoints();
            }
            selectedMappings.clear();
            selectionArea = ofRectangle(0,0,0,0);
        }else{
            if(focusedMappingIndex >= 0 && focusedMappingIndex < mappings.size())
                mappings[focusedMappingIndex]->deselectControlPoint(prevSelectedIndex);
        }
    }
}

void ofxRemoteProjectionMapper::handleKeyReleased(ofKeyEventArgs& args)
{

}

std::shared_ptr<RemoteWarpBase> ofxRemoteProjectionMapper::getWarp(const std::string& name)
{
    auto found = std::find_if( mappings.begin(), mappings.end(), [&name](const std::shared_ptr<RemoteWarpBase>& warp){
        return warp->getName() == name;
    });
    if(found != mappings.end()){
        return *found;
    }else{
        ofLogWarning() << "couldn't find mapping with name " << name;
        return nullptr;
    }
}

void ofxRemoteProjectionMapper::toggleEditing()
{
    for(auto& map: mappings){
        map->setEditing(!map->isEditing());
    }
}

void ofxRemoteProjectionMapper::drawWarps(const ofTexture& tex)
{
    for(auto & warp: mappings){
        warp->drawWarp(tex);
    }
    
    if(selectingMultiple && !selectionAreaSet){
        ofPushStyle();
        ofNoFill();
        ofSetColor(255, 255, 0, 128);
        ofDrawRectangle(selectionArea);
        ofFill();
        ofSetColor(100, 100, 100, 50);
        ofDrawRectangle(selectionArea);
        ofPopStyle();
    }
    
}

void ofxRemoteProjectionMapper::selectControlPoints(const ofRectangle& area)
{
    std::map<size_t, std::vector<size_t>> selectedWarps;
    for (int i = mappings.size() - 1; i >= 0; --i){
        auto selections = mappings[i]->getControlPointsInArea(area);
        if(!selections.empty()){
            selectedWarps.emplace( i, std::move(selections) );
        }
    }
    
    for(auto & found: selectedWarps){
        selectedMappings.push_back(found.first);
        for(auto & ind : found.second){
            mappings[found.first]->selectControlPoint(ind);
        }
    }
}

//--------------------------------------------------------------
void ofxRemoteProjectionMapper::selectClosestControlPoint(int x, int y)
{
    size_t warpIdx = -1;
    size_t pointIdx = -1;
    auto distance = std::numeric_limits<float>::max();
    
    // Find warp and distance to closest control point.
    for (int i = mappings.size() - 1; i >= 0; --i)
    {
        float candidate;
        auto idx = mappings[i]->findClosestControlPoint(glm::vec2(x,y), &candidate);
        if (candidate < distance && mappings[i]->isEditing())
        {
            distance = candidate;
            pointIdx = idx;
            warpIdx = i;
        }
    }
    
    if(warpIdx == focusedMappingIndex && prevSelectedIndex == pointIdx )
        return;
    
    if(focusedMappingIndex < mappings.size() && focusedMappingIndex >=0)
        mappings[focusedMappingIndex]->deselectControlPoint(prevSelectedIndex);
    if(warpIdx < mappings.size() && warpIdx >= 0 )
        mappings[warpIdx]->selectControlPoint(pointIdx);
    
    focusedMappingIndex = warpIdx;
    prevSelectedIndex = pointIdx;
}

void ofxRemoteProjectionMapper::handleMouseDown(ofMouseEventArgs& args)
{
    if(!selectingMultiple){
        // Find and select closest control point.
        selectClosestControlPoint(args.x,args.y);
        
        if (focusedMappingIndex < mappings.size())
        {
            mappings[focusedMappingIndex]->handleCursorDown(args);
        }
    }else{
        if(!selectionAreaSet){
            selectionArea = ofRectangle(args, args);
        }else{
            for(auto & selected: selectedMappings){
                mappings[selected]->handleCursorDown(args);
            }
        }
    }
}

void ofxRemoteProjectionMapper::handleMouseMove(ofMouseEventArgs& args)
{
    if(!selectingMultiple){
        selectClosestControlPoint(args.x,args.y);
    }
}

void ofxRemoteProjectionMapper::handleMouseDrag(ofMouseEventArgs& args)
{
    if(selectingMultiple){
        if(!selectionAreaSet){
            selectionArea = ofRectangle( selectionArea.getTopLeft(), args );
        }else{
            for(auto & selected: selectedMappings){
                mappings[selected]->handleCursorDrag(args);
            }
        }
    }else{
        if (focusedMappingIndex < mappings.size())
        {
            mappings[focusedMappingIndex]->handleCursorDrag(args);
        }
    }
}

void ofxRemoteProjectionMapper::handleMouseUp(ofMouseEventArgs& args)
{
    if(selectingMultiple && !selectionAreaSet){
        selectionAreaSet = true;
        selectControlPoints(selectionArea);
    }
}

void ofxRemoteProjectionMapper::handleWindowResize(int width, int height)
{
    for(auto & warp: mappings){
        warp->handleWindowResize(width, height);
    }
}

void ofxRemoteProjectionMapper::createPerspectiveWarp()
{
    mappings.emplace_back(std::make_shared<RemoteWarpPerspective>(nextWarpName,
                                                                  WarpSettings()
                                                                  .srcSize(contentSize.x, contentSize.y)
                                                                  .saveLocation(saveLocation)
                                                                  .srcArea(ofRectangle(0,0,contentSize.x,contentSize.y))
                                                                  .drawArea(ofRectangle(0,0,ofGetWidth(),ofGetHeight()))
                                                                  ));
    lastWarpName = nextWarpName;
    saveWarps();
}

void ofxRemoteProjectionMapper::createBiliearWarp()
{
    mappings.emplace_back(std::make_shared<RemoteWarpBilinear>(nextWarpName,
                                                               WarpSettings()
                                                               .srcSize(contentSize.x, contentSize.y)
                                                               .saveLocation(saveLocation)
                                                               .srcArea(ofRectangle(0,0,contentSize.x,contentSize.y))
                                                               .drawArea(ofRectangle(0,0,ofGetWidth(),ofGetHeight()))
                                                               ));
    lastWarpName = nextWarpName;
    saveWarps();
}

void ofxRemoteProjectionMapper::createPerspectiveBilinearWarp()
{
    mappings.emplace_back(std::make_shared<RemoteWarpPerspectiveBilinear>(nextWarpName,
                                                                          WarpSettings()
                                                                          .srcSize(contentSize.x, contentSize.y)
                                                                          .saveLocation(saveLocation)
                                                                          .srcArea(ofRectangle(0,0,contentSize.x,contentSize.y))
                                                                          .drawArea(ofRectangle(0,0,ofGetWidth(),ofGetHeight()))
                                                                          ));
    lastWarpName = nextWarpName;
    saveWarps();
}

void ofxRemoteProjectionMapper::handleRemoteUpdate(RemoteUIServerCallBackArg & arg)
{
    switch (arg.action) {
        case CLIENT_UPDATED_PARAM:
            if(arg.group == "Mapper"){
                if(arg.paramName == "next warp name"){
                    nextWarpName = arg.param.getValueAsString();
                }else if(arg.paramName == "create persp warp"){
                    if(nextWarpName == lastWarpName)
                        return;
                    if(doCreatePerspectiveWarp){
                        createPerspectiveWarp();
                        doCreatePerspectiveWarp = false;
                        RUI_PUSH_TO_CLIENT();
                    }
                }else if(arg.paramName == "create bilinear warp"){
                    if(nextWarpName == lastWarpName)
                        return;
                    if(doCreateBilinearWarp){
                        createBiliearWarp();
                        doCreateBilinearWarp = false;
                        RUI_PUSH_TO_CLIENT();
                    }
                }else if(arg.paramName == "create perp bilinear warp"){
                    if(nextWarpName == lastWarpName)
                        return;
                    if(doCreatePerspectiveBilinearWarp){
                        createPerspectiveBilinearWarp();
                        doCreatePerspectiveBilinearWarp = false;
                        RUI_PUSH_TO_CLIENT();
                    }
                }
            }
            break;
        default: break;
    }
}

void ofxRemoteProjectionMapper::loadWarps()
{
    auto infile = ofFile(saveLocation/"ProjectionMapping.json", ofFile::ReadOnly);
    if (!infile.exists())
    {
        ofLogWarning("RemoteProjectionMapper::loadConfig") << "File not found at path " << saveLocation;
        return false;
    }
    
    nlohmann::json json;
    infile >> json;
    
    for (const auto & warpJson : json["warps"])
    {
        std::string name = warpJson["name"];
        int type = warpJson["type"];
        
        glm::ivec2 srcSize;
        std::istringstream iss;
        iss.str(warpJson["srcSize"]);
        iss >> srcSize;
        
        auto& srcAreaJson =  warpJson["srcArea"];
        ofRectangle srcArea(srcAreaJson["x"].get<float>(),
                            srcAreaJson["y"].get<float>(),
                            srcAreaJson["w"].get<float>(),
                            srcAreaJson["h"].get<float>());
        
        auto& drawAreaJson =  warpJson["drawArea"];
        ofRectangle drawArea(drawAreaJson["x"].get<float>(),
                            drawAreaJson["y"].get<float>(),
                            drawAreaJson["w"].get<float>(),
                            drawAreaJson["h"].get<float>());
        
        switch(type){
            case WarpSettings::TYPE_PERSPECTIVE:
            {
                mappings.emplace_back(std::make_shared<RemoteWarpPerspective>(name,
                                                                              WarpSettings()
                                                                              .saveLocation(saveLocation)
                                                                              .srcSize(srcSize.x, srcSize.y)
                                                                              .drawArea(drawArea)
                                                                              .srcArea(srcArea)
                                                                              ));
                mappings.back()->deserialize(warpJson["warp"]);
            }break;
            case WarpSettings::TYPE_BILINEAR:
            {
                mappings.emplace_back(std::make_shared<RemoteWarpBilinear>(name,
                                                                              WarpSettings()
                                                                              .saveLocation(saveLocation)
                                                                              .srcSize(srcSize.x, srcSize.y)
                                                                              .drawArea(drawArea)
                                                                              .srcArea(srcArea)
                                                                              ));
                mappings.back()->deserialize(warpJson["warp"]);
            }break;
            case WarpSettings::TYPE_PERSPECTIVE_BILINEAR:
            {
                mappings.emplace_back(std::make_shared<RemoteWarpPerspectiveBilinear>(name,
                                                                              WarpSettings()
                                                                              .saveLocation(saveLocation)
                                                                              .srcSize(srcSize.x, srcSize.y)
                                                                              .drawArea(drawArea)
                                                                              .srcArea(srcArea)
                                                                              ));
                mappings.back()->deserialize(warpJson["warp"]);
            }break;
            default:
                ofLogError() << "RemoteProjectionMapper::loadConfig | UNKNOWN WARP TYPE";
                break;
        }

    }
    
}

void ofxRemoteProjectionMapper::saveWarps()
{
    nlohmann::json out;
    
    auto& warps = out["warps"];
    
    for(auto & mapping: mappings){
        nlohmann::json warp;
        warp["name"] = mapping->getName();
        warp["type"] = mapping->getType();

        std::ostringstream oss;
        oss << mapping->getSrcSize();
        warp["srcSize"] = oss.str();
        
        auto& srcArea =  warp["srcArea"];
        srcArea["x"] = mapping->getSrcArea().x;
        srcArea["y"] = mapping->getSrcArea().y;
        srcArea["w"] = mapping->getSrcArea().width;
        srcArea["h"] = mapping->getSrcArea().height;

        auto& drawArea =  warp["drawArea"];
        drawArea["x"] = mapping->getDrawArea().x;
        drawArea["y"] = mapping->getDrawArea().y;
        drawArea["w"] = mapping->getDrawArea().width;
        drawArea["h"] = mapping->getDrawArea().height;
        
        nlohmann::json serialized;
        mapping->serialize(serialized);
        warp["warp"] = serialized;
        
        warps.push_back(warp);
    }
    
    auto outFile = ofFile(saveLocation/"ProjectionMapping.json", ofFile::WriteOnly);
    outFile << out.dump(4);
}


