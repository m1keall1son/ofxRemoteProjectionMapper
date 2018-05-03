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
    
    loadWarps();
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

void ofxRemoteProjectionMapper::drawWarps(const ofTexture& tex)
{
    for(auto & warp: mappings){
        warp->drawWarp(tex);
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
        
        warps.push_back(warp);
    }
    
    auto outFile = ofFile(saveLocation/"ProjectionMapping.json", ofFile::WriteOnly);
    outFile << out.dump(4);
}


