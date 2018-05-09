//
//  RemoteProjectionMapper.hpp
//  RemoteProjectionMapper
//
//  Created by Michael Allison on 5/1/18.
//

#pragma once

#include "RemoteWarpBase.h"
#include "RemoteWarpPerspectiveBilinear.h"
#include "RemoteWarpPerspective.h"
#include "RemoteWarpBilinear.h"

#include <type_traits>
#include <memory>

class ofxRemoteProjectionMapper {
public:
    
    ofxRemoteProjectionMapper();
    ~ofxRemoteProjectionMapper();
    
    //initializes remote UI options and optionally remote UI server iteself, inits the server by default
    void init(bool initRemoteUI = true, int port = -1, float updateInterval = .1, bool verbose = true);
    
    //draw all warps using the provided texture
    void drawWarps(const ofTexture& tex);
    
    //load created warps created remotely from file
    void loadWarps();
    
    //write warps created remotely to file
    void saveWarps();
    
    //explicitly handle a resize
    void handleWindowResize(int width, int height);
    
    //get a single warp by name, useful if there are multiple warps that draw different textures
    std::shared_ptr<RemoteWarpBase> getWarp(const std::string& name);
    
    //manually create a warp: names must be unique, if the warp already exists it will return it.
    template<typename WarpType, typename...Args>
    std::shared_ptr<WarpType> createWarp( const std::string& name, const WarpSettings& settings, Args&&...args )
    {
        static_assert( std::is_base_of<RemoteWarpBase, WarpType>::value, "WarpType must inherit RemoteWarpBase!");
        auto found = std::find_if( mappings.begin(), mappings.end(), [&name](const std::shared_ptr<RemoteWarpBase>& warp){
            return warp->getName() == name;
        });
        if(found != mappings.end()){
            return std::dynamic_pointer_cast<WarpType>(*found);
        }else{
            mappings.emplace_back(std::make_shared<WarpType>( name, settings, std::forward<Args>(args)... ));
            return std::dynamic_pointer_cast<WarpType>(mappings.back());
        }
    }
    
    //set the location of the saved configuration files
    inline void setSaveLocation(const std::filesystem::path& location){ saveLocation = location; }
    
    //set the size of the content to be desiplayed by the next remotely created warps
    inline void setContentSize(int width, int height){ contentSize = glm::ivec2(width, height); }
    
    void selectClosestControlPoint(int x, int y);
    void selectControlPoints(const ofRectangle& area);

private:
    
    void handleMouseDown(ofMouseEventArgs& args);
    void handleMouseDrag(ofMouseEventArgs& args);
    void handleMouseMove(ofMouseEventArgs& args);
    void handleMouseUp(ofMouseEventArgs& args);
    void handleKeyPress(ofKeyEventArgs& args);
    void handleKeyReleased(ofKeyEventArgs& args);

    void createPerspectiveWarp();
    void createBiliearWarp();
    void createPerspectiveBilinearWarp();
    
    void handleRemoteUpdate(RemoteUIServerCallBackArg & arg);
        
    glm::ivec2 contentSize;
    std::vector<std::shared_ptr<RemoteWarpBase>> mappings;
    std::string nextWarpName{"Next Warp"};
    std::string lastWarpName;
    ofRectangle nextWarpSrcArea;
    ofRectangle selectionArea;
    std::filesystem::path saveLocation;
    std::vector<int> selectedMappings;
    int focusedMappingIndex{0};
    int prevSelectedIndex{0};
    bool selectingMultiple{false};
    bool selectionAreaSet{false};
    bool doCreatePerspectiveWarp{false};
    bool doCreateBilinearWarp{false};
    bool doCreatePerspectiveBilinearWarp{false};
};
