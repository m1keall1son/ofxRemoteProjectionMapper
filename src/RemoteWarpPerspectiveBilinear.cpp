//
//  RemoteWarpPerspectiveBilinear.cpp
//  RemoteProjectionMapper
//
//  Created by Michael Allison on 5/1/18.
//

#include "RemoteWarpPerspectiveBilinear.h"
#include "RemoteWarpPerspective.h"

//--------------------------------------------------------------
RemoteWarpPerspectiveBilinear::RemoteWarpPerspectiveBilinear(const std::string& name, const WarpSettings& settings)
: RemoteWarpBilinear(name, settings)
{
    type = WarpSettings::TYPE_PERSPECTIVE_BILINEAR;
    
    // Create child WarpPerspective.
    
    this->srcPoints[0] = glm::vec2(0.0f, 0.0f);
    this->srcPoints[1] = glm::vec2(windowSize.x, 0.0f);
    this->srcPoints[2] = glm::vec2(windowSize.x, windowSize.y);
    this->srcPoints[3] = glm::vec2(0.0f, windowSize.y);
    
    remoteCorners[0] = glm::vec2(0.0f, 0.0f);
    remoteCorners[1] = glm::vec2(1.0f, 0.0f);
    remoteCorners[2] = glm::vec2(1.0f, 1.0f);
    remoteCorners[3] = glm::vec2(0.0f, 1.0f);
    
    if(std::filesystem::exists(saveLocation/currentPreset/RemoteWarpBase::sSaveFilename)){
        loadControlPoints(saveLocation/currentPreset/RemoteWarpBase::sSaveFilename);
        dirty = true;
    }else{
        reset();
    }
}

//--------------------------------------------------------------
RemoteWarpPerspectiveBilinear::~RemoteWarpPerspectiveBilinear()
{
    
}

void RemoteWarpPerspectiveBilinear::handleRemoteUpdate(RemoteUIServerCallBackArg & arg)
{
    switch (arg.action) {
        case CLIENT_UPDATED_PARAM:
            if(arg.group == remoteGroupName){
                static auto c = std::string(ctrlptPrefix+" corner");
                if( arg.paramName.substr(0,c.size()) == c){
                    dirty = true;
                }
            }
            break;
        default:
            break;
    }
    RemoteWarpBilinear::handleRemoteUpdate(arg);
}


void RemoteWarpPerspectiveBilinear::addControlPoints()
{
    RUI_NEW_GROUP(remoteGroupName);
    
    RUI_SHARE_PARAM_WCN(ctrlptPrefix+" corner TL x",remoteCorners[0].x,0.f, 1.f);
    RUI_SHARE_PARAM_WCN(ctrlptPrefix+" corner TL y",remoteCorners[0].y,0.f, 1.f);
    
    RUI_SHARE_PARAM_WCN(ctrlptPrefix+" corner TR x",remoteCorners[1].x,0.f, 1.f);
    RUI_SHARE_PARAM_WCN(ctrlptPrefix+" corner TR y",remoteCorners[1].y,0.f, 1.f);
    
    RUI_SHARE_PARAM_WCN(ctrlptPrefix+" corner BR x",remoteCorners[2].x,0.f, 1.f);
    RUI_SHARE_PARAM_WCN(ctrlptPrefix+" corner BR y",remoteCorners[2].y,0.f, 1.f);
    
    RUI_SHARE_PARAM_WCN(ctrlptPrefix+" corner BL x",remoteCorners[3].x,0.f, 1.f);
    RUI_SHARE_PARAM_WCN(ctrlptPrefix+" corner BL y",remoteCorners[3].y,0.f, 1.f);
    
    RUI_PUSH_TO_CLIENT();
    
    RemoteWarpBilinear::addControlPoints();
}

void RemoteWarpPerspectiveBilinear::removeControlPoints()
{
    auto instance = ofxRemoteUIServer::instance();
    
    instance->removeParamFromDB(ctrlptPrefix + " corner TL x", true);
    instance->removeParamFromDB(ctrlptPrefix + " corner TL y", true);
    
    instance->removeParamFromDB(ctrlptPrefix + " corner TR x", true);
    instance->removeParamFromDB(ctrlptPrefix + " corner TR y", true);
    
    instance->removeParamFromDB(ctrlptPrefix + " corner BR x", true);
    instance->removeParamFromDB(ctrlptPrefix + " corner BR y", true);
    
    instance->removeParamFromDB(ctrlptPrefix + " corner BL x", true);
    instance->removeParamFromDB(ctrlptPrefix + " corner BL y", true);
    
    RUI_PUSH_TO_CLIENT();
    
    RemoteWarpBilinear::removeControlPoints();
}

//--------------------------------------------------------------
void RemoteWarpPerspectiveBilinear::serialize(nlohmann::json & json)
{
    RemoteWarpBilinear::serialize(json);
    
    std::vector<std::string> corners;
    for (auto i = 0; i < 4; ++i)
    {
        const auto corner = remoteCorners[i];
        std::ostringstream oss;
        oss << corner;
        corners.push_back(oss.str());
    }
    json["corners"] = corners;
}

//--------------------------------------------------------------
void RemoteWarpPerspectiveBilinear::deserialize(const nlohmann::json & json)
{
    RemoteWarpBilinear::deserialize(json);
    
    auto i = 0;
    for (const auto & jsonPoint : json["corners"])
    {
        glm::vec2 corner;
        std::istringstream iss;
        iss.str(jsonPoint);
        iss >> corner;
        remoteCorners[i] = corner;
        
        ++i;
    }
    
    if(remoteEditMode)
        RUI_PUSH_TO_CLIENT();
}

//--------------------------------------------------------------
void RemoteWarpPerspectiveBilinear::reset(const glm::vec2 & scale, const glm::vec2 & offset)
{
    remoteCorners[0] = glm::vec2(0.0f, 0.0f);
    remoteCorners[1] = glm::vec2(1.0f, 0.0f);
    remoteCorners[2] = glm::vec2(1.0f, 1.0f);
    remoteCorners[3] = glm::vec2(0.0f, 1.0f);
    
    RemoteWarpBilinear::reset();
}

//--------------------------------------------------------------
glm::vec2 RemoteWarpPerspectiveBilinear::getControlPoint(size_t index)
{
    // Depending on index, return perspective or bilinear control point.
    if (this->isCorner(index))
    {
        // Perspective: simply return one of the corners.
        return remoteCorners[(this->convertIndex(index))];
    }
    else
    {
        // Bilinear: transform control point from warped space to normalized screen space.
        auto cp = RemoteWarpBase::getControlPoint(index) * getSize();
        auto pt = getTransform() * glm::vec4(cp.x, cp.y, 0.0f, 1.0f);
        
        if (pt.w != 0) pt.w = 1.0f / pt.w;
        pt *= pt.w;
        
        return glm::vec2(pt.x, pt.y) / this->windowSize;
    }
}

//--------------------------------------------------------------
void RemoteWarpPerspectiveBilinear::setControlPoint(size_t index, const glm::vec2 & pos)
{
    // Depending on index, set perspective or bilinear control point.
    if (this->isCorner(index))
    {
        // Perspective: simply set the control point.
        remoteCorners[convertIndex(index)] = pos;
    }
    else
    {
        // Bilinear:: transform control point from normalized screen space to warped space.
        auto cp = pos * this->windowSize;
        auto pt = getTransformInverted() * glm::vec4(cp.x, cp.y, 0.0f, 1.0f);
        
        if (pt.w != 0) pt.w = 1.0f / pt.w;
        pt *= pt.w;
        
        RemoteWarpBase::setControlPoint(index, glm::vec2(pt.x, pt.y) / getSize());
    }
}

//--------------------------------------------------------------
void RemoteWarpPerspectiveBilinear::moveControlPoint(size_t index, const glm::vec2 & shift)
{
//    // Depending on index, move perspective or bilinear control point.
//    if (this->isCorner(index))
//    {
//        // Perspective: simply move the control point.
//        this->warpPerspective->moveControlPoint(this->convertIndex(index), shift);
//    }
//    else {
        // Bilinear: transform control point from normalized screen space to warped space.
        auto pt = getControlPoint(index);
        this->setControlPoint(index, pt + shift);
    //}
}

//--------------------------------------------------------------
void RemoteWarpPerspectiveBilinear::selectControlPoint(size_t index)
{
//    // Depending on index, select perspective or bilinear control point.
//    if (this->isCorner(index))
//    {
//        this->warpPerspective->selectControlPoint(this->convertIndex(index));
//    }
//    else
//    {
//        this->warpPerspective->deselectControlPoint();
//    }
    
    // Always select bilinear control point, which we use to keep track of editing.
    RemoteWarpBase::selectControlPoint(index);
}

//--------------------------------------------------------------
void RemoteWarpPerspectiveBilinear::deselectControlPoint()
{
   // this->warpPerspective->deselectControlPoint();
    RemoteWarpBase::deselectControlPoint();
}

//--------------------------------------------------------------
void RemoteWarpPerspectiveBilinear::rotateClockwise()
{
    std::swap(remoteCorners[3], remoteCorners[0]);
    std::swap(remoteCorners[0], remoteCorners[1]);
    std::swap(remoteCorners[1], remoteCorners[2]);
    this->dirty = true;
}

//--------------------------------------------------------------
void RemoteWarpPerspectiveBilinear::rotateCounterclockwise()
{
    std::swap(remoteCorners[1], remoteCorners[2]);
    std::swap(remoteCorners[0], remoteCorners[1]);
    std::swap(remoteCorners[3], remoteCorners[0]);
    this->dirty = true;
}

//--------------------------------------------------------------
bool RemoteWarpPerspectiveBilinear::handleCursorDown(const glm::vec2 & pos)
{
    if (!this->editing || this->selectedIndex >= this->controlPoints.size()) return false;
    
//    // Depending on selected control point, let perspective or bilinear warp handle it.
//    if (this->isCorner(this->selectedIndex))
//    {
//        return this->warpPerspective->handleCursorDown(pos);
//    }
    return RemoteWarpBase::handleCursorDown(pos);
}

//--------------------------------------------------------------
bool RemoteWarpPerspectiveBilinear::handleCursorDrag(const glm::vec2 & pos)
{
    if (!this->editing || this->selectedIndex >= this->controlPoints.size()) return false;
    
//    // Depending on selected control point, let perspective or bilinear warp handle it.
//    if (this->isCorner(this->selectedIndex))
//    {
//        return this->warpPerspective->handleCursorDrag(pos);
//    }
    return RemoteWarpBase::handleCursorDrag(pos);
}

//--------------------------------------------------------------
void RemoteWarpPerspectiveBilinear::drawTexture(const ofTexture & texture, const ofRectangle & srcBounds, const ofRectangle & dstBounds)
{
    ofPushMatrix();
    {
        // Apply Perspective transform.
        ofMultMatrix(this->getTransform());
        
        // Draw Bilinear warp.
        RemoteWarpBilinear::drawTexture(texture, srcBounds, dstBounds);
    }
    ofPopMatrix();
}

//--------------------------------------------------------------
bool RemoteWarpPerspectiveBilinear::isCorner(size_t index) const
{
    auto numControls = (this->numControlsX * this->numControlsY);
    
    return (index == 0 || index == (numControls - this->numControlsY) || index == (numControls - 1) || index == (this->numControlsY - 1));
}

//--------------------------------------------------------------
size_t RemoteWarpPerspectiveBilinear::convertIndex(size_t index) const
{
    auto numControls = (this->numControlsX * this->numControlsY);
    
    if (index == 0)
    {
        return 0;
    }
    if (index == (numControls - this->numControlsY))
    {
        return 2;
    }
    if (index == (numControls - 1))
    {
        return 3;
    }
    if (index == (this->numControlsY - 1))
    {
        return 1;
    }
    
    return index;
}

void RemoteWarpPerspectiveBilinear::drawWarp(const ofTexture& tex)
{
    if(show){
        ofPushMatrix();
        ofTranslate(drawArea.x, drawArea.y);
        draw(tex, srcArea);
        ofPopMatrix();
        if(remoteEditMode){
            ofPushMatrix();
            ofMultMatrix(getTransform());
            drawControlPointNames();
            ofPopMatrix();
        }
    }
}


//--------------------------------------------------------------
const glm::mat4 & RemoteWarpPerspectiveBilinear::getTransform()
{
    // Calculate warp matrix.
    if (this->dirty) {
        // Update source size.
        this->srcPoints[1].x = windowSize.x;
        this->srcPoints[2].x = windowSize.x;
        this->srcPoints[2].y = windowSize.y;
        this->srcPoints[3].y = windowSize.y;
        
        // Convert corners to actual destination pixels.
        for (int i = 0; i < 4; ++i)
        {
            this->dstPoints[i] = remoteCorners[i] * this->windowSize;
        }
        
        // Calculate warp matrix.
        this->transform = PerspectiveTransformation::transform(this->srcPoints, this->dstPoints);
        this->transformInverted = glm::inverse(this->transform);
        
    }
    
    return this->transform;
}

//--------------------------------------------------------------
const glm::mat4 & RemoteWarpPerspectiveBilinear::getTransformInverted()
{
    if (this->dirty)
    {
        this->getTransform();
    }
    
    return this->transformInverted;
}


