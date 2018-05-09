//
//  RemoteWarpPerspectiveBilinear.hpp
//  RemoteProjectionMapper
//
//  Created by Michael Allison on 5/1/18.
//

#pragma once

#include "RemoteWarpBilinear.h"

class RemoteWarpPerspectiveBilinear : public RemoteWarpBilinear {
public:
    RemoteWarpPerspectiveBilinear(const std::string& name, const WarpSettings& settings);
    virtual ~RemoteWarpPerspectiveBilinear();
    
    virtual void serialize(nlohmann::json & json) override;
    virtual void deserialize(const nlohmann::json & json) override;
    
    const glm::mat4 & getTransform();
    const glm::mat4 & getTransformInverted();
    
    //! reset control points to undistorted image
    virtual void reset(const glm::vec2 & scale = glm::vec2(1.0f), const glm::vec2 & offset = glm::vec2(0.0f)) override;
    
    //! return the coordinates of the specified control point
    virtual glm::vec2 getControlPoint(size_t index) override;
    //! set the coordinates of the specified control point
    virtual void setControlPoint(size_t index, const glm::vec2 & pos) override;
    //! move the specified control point
    virtual void moveControlPoint(size_t index, const glm::vec2 & shift) override;
    //! select one of the control points
    virtual void selectControlPoint(size_t index) override;
    //! deselect the selected control point
    virtual void deselectControlPoint(size_t index) override;
    
    virtual void rotateClockwise() override;
    virtual void rotateCounterclockwise() override;
    
    virtual bool handleCursorDown(const glm::vec2 & pos) override;
    virtual bool handleCursorDrag(const glm::vec2 & pos) override;
        
    virtual void drawWarp(const ofTexture& tex)override;
    
protected:
    
    virtual void addControlPoints()override;
    virtual void removeControlPoints()override;
    
    //! draw a specific area of a warped texture to a specific region
    virtual void drawTexture(const ofTexture & texture, const ofRectangle & srcBounds, const ofRectangle & dstBounds) override;
    
    //! return whether or not the control point is one of the 4 corners and should be treated as a perspective control point
    bool isCorner(size_t index) const;
    //! convert the control point index to the appropriate perspective warp index
    size_t convertIndex(size_t index) const;
    
    virtual void handleRemoteUpdate(RemoteUIServerCallBackArg & arg)override;
        
    glm::vec2 srcPoints[4];
    glm::vec2 dstPoints[4];
    
    glm::mat4 transform;
    glm::mat4 transformInverted;
    glm::vec2 perspSize;
        
    glm::vec2 remoteCorners[4];
    
};
