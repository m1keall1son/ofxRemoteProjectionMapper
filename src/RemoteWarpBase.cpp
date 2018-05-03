//
//  RemoteWarpBase.cpp
//  RemoteProjectionMapper
//
//  Created by Michael Allison on 5/1/18.
//

#include "RemoteWarpBase.h"
#include "ofxRemoteUIServer.h"
#include "ofMain.h"

std::string RemoteWarpBase::sSaveFilename = "controlpoints.json";

RemoteWarpBase::RemoteWarpBase(const std::string& name, const WarpSettings& settings) :
    type(settings._type),
    editing(false),
    dirty(true),
    brightness(1.0f),
    width(640.0f),
    height(480.0f),
    numControlsX(2),
    numControlsY(2),
    selectedIndex(0),
    selectedTime(0.0f),
    luminance(0.5f),
    gamma(1.0f),
    exponent(2.0f),
    edges(0.0f),
    warpName(name),
    saveLocation(settings._saveLocation),
    srcArea(settings._srcArea),
    drawArea(settings._drawArea),
    show(true)
{
    width = settings._width;
    height = settings._height;
    brightness = settings._brightness;
    luminance = settings._luminance;
    gamma = settings._gamma;
    edges = settings._edges;
    exponent = settings._exponent;
    
    windowSize = glm::vec2(settings._drawArea.width, settings._drawArea.height);
    
    dirty = true;
    
    ofAddListener(RUI_GET_OF_EVENT(), this, &RemoteWarpBase::handleRemoteUpdate);
    
    remoteGroupName = "[warp] "+warpName;
    ctrlptPrefix = warpName+"-cp";
    currentPreset = "no_preset";
    
    if(std::filesystem::exists(saveLocation)){
        if(!std::filesystem::is_directory(saveLocation)){
            throw std::runtime_error("save location must be a directory!");
        }
        if(!std::filesystem::exists(saveLocation/warpName)){
            std::filesystem::create_directory(saveLocation/warpName);
        }else{
            if(!std::filesystem::exists(saveLocation/warpName/"no_preset")){
                std::filesystem::create_directory(saveLocation/warpName/"no_preset");
            }
        }
    }else{
        std::filesystem::create_directory(saveLocation);
        std::filesystem::create_directory(saveLocation/warpName);
        std::filesystem::create_directory(saveLocation/warpName/"no_preset");
    }
    
    saveLocation = saveLocation/warpName;

    RUI_NEW_COLOR();
    RUI_NEW_GROUP(remoteGroupName);
    
    RUI_SHARE_PARAM_WCN(warpName+"-save",saveGroup);
    RUI_SHARE_PARAM_WCN(warpName+"-show",show);
    RUI_SHARE_PARAM_WCN(warpName+"-width",width, 0, windowSize.x);
    RUI_SHARE_PARAM_WCN(warpName+"-height",height,0, windowSize.y);
    RUI_SHARE_PARAM_WCN(warpName+"-src x",srcArea.x, 0, windowSize.x);
    RUI_SHARE_PARAM_WCN(warpName+"-src y",srcArea.y, 0, windowSize.y);
    RUI_SHARE_PARAM_WCN(warpName+"-src width",srcArea.width, 0, windowSize.x);
    RUI_SHARE_PARAM_WCN(warpName+"-src height",srcArea.height, 0, windowSize.y);
    RUI_SHARE_PARAM_WCN(warpName+"-draw x",drawArea.x, 0, ofGetWidth());
    RUI_SHARE_PARAM_WCN(warpName+"-draw y",drawArea.y, 0, ofGetHeight());
    RUI_SHARE_PARAM_WCN(warpName+"-draw width",windowSize.x, 0, ofGetWidth());
    RUI_SHARE_PARAM_WCN(warpName+"-draw height",windowSize.y, 0, ofGetHeight());
    RUI_SHARE_PARAM_WCN(warpName+"-brightness", brightness,0.0,1.0);
    RUI_SHARE_PARAM_WCN(warpName+"-luminance red", luminance.r, 0., 1.);
    RUI_SHARE_PARAM_WCN(warpName+"-luminance green", luminance.g, 0., 1.);
    RUI_SHARE_PARAM_WCN(warpName+"-luminance blue", luminance.b, 0., 1.);
    RUI_SHARE_PARAM_WCN(warpName+"-gamma red", gamma.r, 0., 1.);
    RUI_SHARE_PARAM_WCN(warpName+"-gamma green", gamma.g, 0., 1.);
    RUI_SHARE_PARAM_WCN(warpName+"-gamma blue", gamma.b, 0., 1.);
    RUI_SHARE_PARAM_WCN(warpName+"-edge exponent", exponent, 1., 10.);
    RUI_SHARE_PARAM_WCN(warpName+"-edge left", edges.x, 0., 1.);
    RUI_SHARE_PARAM_WCN(warpName+"-edge top", edges.y, 0., 1.);
    RUI_SHARE_PARAM_WCN(warpName+"-edge right", edges.z, 0., 1.);
    RUI_SHARE_PARAM_WCN(warpName+"-edge bottom", edges.w, 0., 1.);
    RUI_SHARE_PARAM_WCN(warpName+"-editMesh",remoteEditMode);
    
    ofxRemoteUIServer::instance()->addParamToPresetLoadIgnoreList(warpName+"-editMesh");
    
}

void RemoteWarpBase::handleRemoteUpdate(RemoteUIServerCallBackArg & arg)
{    
    switch (arg.action) {
        case CLIENT_UPDATED_PARAM:
            if(arg.group == remoteGroupName){
                if(arg.paramName == (warpName+"-width") ||
                   arg.paramName == (warpName+"-height") ||
                   arg.paramName == (warpName+"-draw width") ||
                   arg.paramName == (warpName+"-draw height")
                   ){
                    dirty = true;
                }else if(arg.paramName == (warpName+"-save")){
                    saveControlPoints(saveLocation/currentPreset/sSaveFilename);
                    saveGroup = false;
                    RUI_PUSH_TO_CLIENT();
                }else if(arg.paramName == (warpName+"-editMesh")){
                    setEditing(remoteEditMode);
                    if(remoteEditMode){
                        addControlPoints();
                    }else{
                        removeControlPoints();
                        saveControlPoints(saveLocation/currentPreset/sSaveFilename);
                    }
                }
            }
            break;
            
        case CLIENT_DID_SET_PRESET:
        {
            currentPreset = arg.msg;
            loadControlPoints(saveLocation/currentPreset/sSaveFilename);
        }break;
        case CLIENT_SAVED_PRESET:{
            if(currentPreset != arg.msg){
                currentPreset = arg.msg;
            }
            if(!std::filesystem::exists(saveLocation/currentPreset)){
                std::filesystem::create_directory(saveLocation/currentPreset);
            }
            saveControlPoints(saveLocation/currentPreset/sSaveFilename);
        }break;
        case CLIENT_DELETED_PRESET:
        {
            std::filesystem::remove_all(saveLocation/arg.msg/sSaveFilename);
            if(arg.msg == currentPreset){
                currentPreset = "no_preset";
            }
        }break;
        case CLIENT_DID_SET_GROUP_PRESET:{
            if(arg.group == remoteGroupName){
                currentPreset = arg.msg;
                loadControlPoints(saveLocation/currentPreset/sSaveFilename);
            }
        }break;
        case CLIENT_SAVED_GROUP_PRESET:{
            if(arg.group == remoteGroupName){
                if(currentPreset != arg.msg){
                    currentPreset = arg.msg;
                }
                if(!std::filesystem::exists(saveLocation/currentPreset)){
                    std::filesystem::create_directory(saveLocation/currentPreset);
                }
                saveControlPoints(saveLocation/currentPreset/sSaveFilename);
            }
        }break;
        case CLIENT_DELETED_GROUP_PRESET:{
            if(arg.group == remoteGroupName){
                std::filesystem::remove_all(saveLocation/arg.msg/sSaveFilename);
                if(arg.msg == currentPreset){
                    currentPreset = "no_preset";
                }
            }
        }break;
        case SERVER_DID_PROGRAMATICALLY_LOAD_PRESET:{
            currentPreset = arg.msg;
            loadControlPoints(saveLocation/currentPreset/sSaveFilename);
        }break;
        case CLIENT_SAVED_STATE:{
            saveControlPoints(saveLocation/currentPreset/sSaveFilename);
        }break;
        case CLIENT_DID_RESET_TO_XML:{
            loadControlPoints(saveLocation/"no_preset"/sSaveFilename);
        }break;
        case CLIENT_DID_RESET_TO_DEFAULTS:{
            loadControlPoints(saveLocation/"no_preset"/sSaveFilename);
        }break;
        default:
            break;
    }
}

void RemoteWarpBase::addControlPoints()
{
    int i = 0;
    RUI_NEW_GROUP(remoteGroupName);
    for(auto& cp : controlPoints){
        RUI_SHARE_PARAM_WCN(ctrlptPrefix+ofToString(i)+" x",cp.x,0.f, 1.f);
        RUI_SHARE_PARAM_WCN(ctrlptPrefix+ofToString(i++)+" y",cp.y,0.f, 1.f);
    }
    RUI_PUSH_TO_CLIENT();
}

void RemoteWarpBase::removeControlPoints()
{
    for(int i =0;i<controlPoints.size();i++){
        auto instance = ofxRemoteUIServer::instance();
        instance->removeParamFromDB(ctrlptPrefix + ofToString(i) + " x", true);
        instance->removeParamFromDB(ctrlptPrefix + ofToString(i) + " y", true);
    }
    RUI_PUSH_TO_CLIENT();
}

void RemoteWarpBase::serialize(nlohmann::json & json)
{
    json["name"] = warpName;
    json["preset"] = currentPreset;
    json["type"] = type;
    json["brightness"] = brightness;
    // Warp parameters.
    {
        auto & jsonWarp = json["warp"];
        
        jsonWarp["columns"] = numControlsX;
        jsonWarp["rows"] = numControlsY;
        
        std::vector<std::string> points;
        for (auto & controlPoint : controlPoints)
        {
            std::ostringstream oss;
            oss << controlPoint;
            points.push_back(oss.str());
        }
        jsonWarp["control points"] = points;
    }
    
    // Blend parameters.
    {
        auto & jsonBlend = json["blend"];
        
        jsonBlend["exponent"] = exponent;
        
        std::ostringstream oss;
        oss << edges;
        jsonBlend["edges"] = oss.str();
        
        oss.str("");
        oss << gamma;
        jsonBlend["gamma"] = oss.str();
        
        oss.str("");
        oss << luminance;
        jsonBlend["luminance"] = oss.str();
    }
}

void RemoteWarpBase::deserialize(const nlohmann::json & json)
{
    auto name = json["name"].get<std::string>();
    auto preset = json["preset"].get<std::string>();
   
    if(name == warpName && preset == currentPreset){
        
        int typeAsInt = json["type"];
        type = (WarpSettings::Type)typeAsInt;
        //brightness = json["brightness"];
        
        // Warp parameters.
        {
            const auto & jsonWarp = json["warp"];
            
            numControlsX = jsonWarp["columns"];
            numControlsY = jsonWarp["rows"];
            
            controlPoints.clear();
            for (const auto & jsonPoint : jsonWarp["control points"])
            {
                glm::vec2 controlPoint;
                std::istringstream iss;
                iss.str(jsonPoint);
                iss >> controlPoint;
                controlPoints.push_back(controlPoint);
            }
        }
        
        // Blend parameters.
//        {
//            const auto & jsonBlend = json["blend"];
//            
//            exponent = jsonBlend["exponent"];
//            
//            {
//                std::istringstream iss;
//                iss.str(jsonBlend["edges"]);
//                iss >> edges;
//            }
//            {
//                std::istringstream iss;
//                iss.str(jsonBlend["gamma"]);
//                iss >> gamma;
//            }
//            {
//                std::istringstream iss;
//                iss.str(jsonBlend["luminance"]);
//                iss >> luminance;
//            }
//        }
        
        dirty = true;
    }else{
        ofLogError() << "Name doesn't match, loaded: " << name << " expected: " << warpName << "or preset doesn't match, loaded: " << currentPreset << " expected: " << preset;
    }
}

RemoteWarpBase::~RemoteWarpBase()
{
    if(remoteEditMode){
        removeControlPoints();
        remoteEditMode = false;
        RUI_PUSH_TO_CLIENT();
    }
}

void RemoteWarpBase::saveControlPoints(const std::filesystem::path& file)
{
    nlohmann::json json;
    serialize(json);
    auto out = ofFile(file, ofFile::WriteOnly);
    out << json.dump(4);
}

void RemoteWarpBase::loadControlPoints(const std::filesystem::path& file)
{
    auto infile = ofFile(file, ofFile::ReadOnly);
    if (!infile.exists())
    {
        ofLogWarning("RemoteWarp::loadControlPoints") << "File not found at path " << file;
        return false;
    }
    
    nlohmann::json json;
    infile >> json;
    
    deserialize(json);
}

void RemoteWarpBase::drawWarp(const ofTexture& tex)
{
    if(show){
        ofPushMatrix();
        ofTranslate(drawArea.x, drawArea.y);
        draw(tex, srcArea);
        ofPopMatrix();
        if(remoteEditMode)
            drawControlPointNames();
    }
}

void RemoteWarpBase::queueControlPoint(const glm::vec2 & pos, const ofFloatColor & color, float scale)
{
    if (controlData.size() < MAX_NUM_CONTROL_POINTS)
    {
        controlData.emplace_back(ControlData(pos, ofFloatColor(1.,1.,1.,1.), .25));
    }
}

void RemoteWarpBase::drawControlPointNames()
{
    ofPushStyle();
    ofSetColor(255, 0, 128);
    ofFill();
    int i = 0;
    for(auto& cp : controlPoints){
        auto pos = cp * windowSize;
        ofDrawBitmapString("cp"+ofToString(i++), pos.x+2+drawArea.x, pos.y+2+drawArea.y);
    }
    ofPopStyle();
}

//--------------------------------------------------------------
std::filesystem::path RemoteWarpBase::shaderPath = std::filesystem::path("shaders") / "ofxWarp";

//--------------------------------------------------------------
void RemoteWarpBase::setShaderPath(const std::filesystem::path shaderPath)
{
    RemoteWarpBase::shaderPath = shaderPath;
}

//--------------------------------------------------------------
WarpSettings::Type RemoteWarpBase::getType() const
{
    return type;
}

//--------------------------------------------------------------
void RemoteWarpBase::setEditing(bool editing)
{
    this->editing = editing;
}

//--------------------------------------------------------------
void RemoteWarpBase::toggleEditing()
{
    setEditing(!editing);
}

//--------------------------------------------------------------
bool RemoteWarpBase::isEditing() const
{
    return editing;
}

//--------------------------------------------------------------
void RemoteWarpBase::setWidth(float width)
{
    setSize(width, height);
}

//--------------------------------------------------------------
float RemoteWarpBase::getWidth() const
{
    return width;
}

//--------------------------------------------------------------
void RemoteWarpBase::setHeight(float height)
{
    setSize(width, height);
}

//--------------------------------------------------------------
float RemoteWarpBase::getHeight() const
{
    return height;
}

//--------------------------------------------------------------
void RemoteWarpBase::setSize(float width, float height)
{
    width = width;
    height = height;
    dirty = true;
}

//--------------------------------------------------------------
void RemoteWarpBase::setSize(const glm::vec2 & size)
{
    setSize(size.x, size.y);
}

//--------------------------------------------------------------
glm::vec2 RemoteWarpBase::getSize() const
{
    return glm::vec2(width, height);
}

//--------------------------------------------------------------
ofRectangle RemoteWarpBase::getBounds() const
{
    return ofRectangle(0, 0, width, height);
}

//--------------------------------------------------------------
void RemoteWarpBase::setBrightness(float brightness)
{
    brightness = brightness;
}

//--------------------------------------------------------------
float RemoteWarpBase::getBrightness() const
{
    return brightness;
}

//--------------------------------------------------------------
void RemoteWarpBase::setLuminance(float lum)
{
    luminance = glm::vec3(lum);
}

//--------------------------------------------------------------
void RemoteWarpBase::setLuminance(float red, float green, float blue)
{
    luminance = glm::vec3(red, green, blue);
}

//--------------------------------------------------------------
void RemoteWarpBase::setLuminance(const glm::vec3 & rgb)
{
    luminance = rgb;
}

//--------------------------------------------------------------
const glm::vec3 & RemoteWarpBase::getLuminance() const
{
    return luminance;
}

//--------------------------------------------------------------
void RemoteWarpBase::setGamma(float g)
{
    gamma = glm::vec3(g);
}

//--------------------------------------------------------------
void RemoteWarpBase::setGamma(float red, float green, float blue)
{
    gamma = glm::vec3(red, green, blue);
}

//--------------------------------------------------------------
void RemoteWarpBase::setGamma(const glm::vec3 & rgb)
{
    gamma = rgb;
}

//--------------------------------------------------------------
const glm::vec3 & RemoteWarpBase::getGamma() const
{
    return gamma;
}

//--------------------------------------------------------------
void RemoteWarpBase::setExponent(float exponent)
{
    exponent = exponent;
}

//--------------------------------------------------------------
float RemoteWarpBase::getExponent() const
{
    return exponent;
}

//--------------------------------------------------------------
void RemoteWarpBase::setEdges(float left, float top, float right, float bottom)
{
    setEdges(glm::vec4(left, top, right, bottom));
}

//--------------------------------------------------------------
void RemoteWarpBase::setEdges(const glm::vec4 & e)
{
    edges.x = ofClamp(e.x * 0.5f, 0.0f, 1.0f);
    edges.y = ofClamp(e.y * 0.5f, 0.0f, 1.0f);
    edges.z = ofClamp(e.z * 0.5f, 0.0f, 1.0f);
    edges.w = ofClamp(e.w * 0.5f, 0.0f, 1.0f);
}

//--------------------------------------------------------------
glm::vec4 RemoteWarpBase::getEdges() const
{
    return edges * 2.0f;
}

//--------------------------------------------------------------
void RemoteWarpBase::draw(const ofTexture & texture)
{
    draw(texture, ofRectangle(0, 0, texture.getWidth(), texture.getHeight()), getBounds());
}

//--------------------------------------------------------------
void RemoteWarpBase::draw(const ofTexture & texture, const ofRectangle & srcBounds)
{
    draw(texture, srcBounds, getBounds());
}

//--------------------------------------------------------------
void RemoteWarpBase::draw(const ofTexture & texture, const ofRectangle & srcBounds, const ofRectangle & dstBounds)
{
    drawTexture(texture, srcBounds, dstBounds);
    drawControls();
}

//--------------------------------------------------------------
bool RemoteWarpBase::clip(ofRectangle & srcBounds, ofRectangle & dstBounds) const
{
    bool clipped = false;
    
    glm::vec4 srcVec = glm::vec4(srcBounds.getMinX(), srcBounds.getMinY(), srcBounds.getMaxX(), srcBounds.getMaxY());
    glm::vec4 dstVec = glm::vec4(dstBounds.getMinX(), dstBounds.getMinY(), dstBounds.getMaxX(), dstBounds.getMaxY());
    
    float x1 = dstVec.x / width;
    float x2 = dstVec.z / width;
    float y1 = dstVec.y / height;
    float y2 = dstVec.w / height;
    
    if (x1 < 0.0f)
    {
        dstVec.x = 0.0f;
        srcVec.x -= (x1 * srcBounds.getWidth());
        clipped = true;
    }
    else if (x1 > 1.0f)
    {
        dstVec.x = width;
        srcVec.x -= ((1.0f / x1) * srcBounds.getWidth());
        clipped = true;
    }
    
    if (x2 < 0.0f)
    {
        dstVec.z = 0.0f;
        srcVec.z -= (x2 * srcBounds.getWidth());
        clipped = true;
    }
    else if (x2 > 1.0f) {
        dstVec.z = width;
        srcVec.z -= ((1.0f / x2) * srcBounds.getWidth());
        clipped = true;
    }
    
    if (y1 < 0.0f)
    {
        dstVec.y = 0.0f;
        srcVec.y -= (y1 * srcBounds.getHeight());
        clipped = true;
    }
    else if (y1 > 1.0f)
    {
        dstVec.y = height;
        srcVec.y -= ((1.0f / y1) * srcBounds.getHeight());
        clipped = true;
    }
    
    if (y2 < 0.0f) {
        dstVec.w = 0.0f;
        srcVec.w -= (y2 * srcBounds.getHeight());
        clipped = true;
    }
    else if (y2 > 1.0f) {
        dstVec.w = height;
        srcVec.w -= ((1.0f / y2) * srcBounds.getHeight());
        clipped = true;
    }
    
    srcBounds.set(srcVec.x, srcVec.y, srcVec.z - srcVec.x, srcVec.w - srcVec.y);
    dstBounds.set(dstVec.x, dstVec.y, dstVec.z - dstVec.x, dstVec.w - dstVec.y);
    
    return clipped;
}

//--------------------------------------------------------------
glm::vec2 RemoteWarpBase::getControlPoint(size_t index)
{
    if (index >= controlPoints.size()) return glm::vec2(0.0f);
    
    return controlPoints[index];
}

//--------------------------------------------------------------
void RemoteWarpBase::setControlPoint(size_t index, const glm::vec2 & pos)
{
    if (index >= controlPoints.size()) return;
    
    controlPoints[index] = pos;
    dirty = true;
}

//--------------------------------------------------------------
void RemoteWarpBase::moveControlPoint(size_t index, const glm::vec2 & shift)
{
    if (index >= controlPoints.size()) return;
    
    controlPoints[index] += shift;
    dirty = true;
}

//--------------------------------------------------------------
size_t RemoteWarpBase::getNumControlPoints() const
{
    return controlPoints.size();
}

//--------------------------------------------------------------
size_t RemoteWarpBase::getSelectedControlPoint() const
{
    return selectedIndex;
}

//--------------------------------------------------------------
void RemoteWarpBase::selectControlPoint(size_t index)
{
    if (index >= controlPoints.size() || index == selectedIndex) return;
    
    selectedIndex = index;
    selectedTime = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void RemoteWarpBase::deselectControlPoint()
{
    selectedIndex = -1;
}

//--------------------------------------------------------------
size_t RemoteWarpBase::findClosestControlPoint(const glm::vec2 & pos, float * distance)
{
    size_t index;
    auto minDistance = std::numeric_limits<float>::max();
    
    for (auto i = 0; i < controlPoints.size(); ++i)
    {
        auto candidate = glm::distance(pos, getControlPoint(i) * windowSize);
        if (candidate < minDistance)
        {
            minDistance = candidate;
            index = i;
        }
    }
    
    *distance = minDistance;
    return index;
}

//--------------------------------------------------------------
size_t RemoteWarpBase::getNumControlsX() const
{
    return numControlsX;
}

//--------------------------------------------------------------
size_t RemoteWarpBase::getNumControlsY() const
{
    return numControlsY;
}

//--------------------------------------------------------------
void RemoteWarpBase::queueControlPoint(const glm::vec2 & pos, bool selected, bool attached)
{
    if (selected && attached)
    {
        queueControlPoint(pos, ofFloatColor(0.0f, 0.8f, 0.0f));
    }
    else if (selected)
    {
        auto scale = 0.9f + 0.2f * sinf(6.0f * (ofGetElapsedTimef() - selectedTime));
        queueControlPoint(pos, ofFloatColor(0.9f, 0.9f, 0.9f), scale);
    }
    else if (attached)
    {
        queueControlPoint(pos, ofFloatColor(0.0f, 0.4f, 0.0f));
    }
    else
    {
        queueControlPoint(pos, ofFloatColor(0.4f, 0.4f, 0.4f));
    }
}

//--------------------------------------------------------------
void RemoteWarpBase::setupControlPoints()
{
    if (controlMesh.getVertices().empty())
    {
        // Set up the vbo mesh.
        ofPolyline unitCircle;
        unitCircle.arc(glm::vec3(0.0f), 1.0f, 1.0f, 0.0f, 360.0f, 18);
        const auto & circlePoints = unitCircle.getVertices();
        static const auto radius = 15.0f;
        static const auto halfVec = glm::vec2(0.5f);
        controlMesh.clear();
        controlMesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
        controlMesh.setUsage(GL_STATIC_DRAW);
        controlMesh.addVertex(glm::vec3(0.0f));
        controlMesh.addTexCoord(halfVec);
        for (auto & pt : circlePoints)
        {
            controlMesh.addVertex(pt * radius);
            controlMesh.addTexCoord(glm::vec2(pt) * 0.5f + halfVec);
        }
        
        // Set up per-instance data to the vbo.
        std::vector<ControlData> instanceData;
        instanceData.resize(MAX_NUM_CONTROL_POINTS);
        
        controlMesh.getVbo().setAttributeData(INSTANCE_POS_SCALE_ATTRIBUTE, (float *)&instanceData[0].pos, 4, instanceData.size(), GL_STREAM_DRAW, sizeof(ControlData));
        controlMesh.getVbo().setAttributeDivisor(INSTANCE_POS_SCALE_ATTRIBUTE, 1);
        controlMesh.getVbo().setAttributeData(INSTANCE_COLOR_ATTRIBUTE, (float *)&instanceData[0].color, 4, instanceData.size(), GL_STREAM_DRAW, sizeof(ControlData));
        controlMesh.getVbo().setAttributeDivisor(INSTANCE_COLOR_ATTRIBUTE, 1);
    }
    
    if (!controlShader.isLoaded())
    {
        static const std::string cpVert = OF_GLSL(150,
            // OF default uniforms and attributes
            uniform mat4 modelViewProjectionMatrix;
            uniform vec4 globalColor;
            
            in vec4 position;
            in vec2 texcoord;
            in vec4 color;
            
            // App uniforms and attributes
            in vec4 iPositionScale;
            in vec4 iColor;
            
            out vec2 vTexCoord;
            out vec4 vColor;
            
            void main(void)
            {
                vTexCoord = texcoord;
                vColor = globalColor * iColor;
                gl_Position = modelViewProjectionMatrix * vec4(position.xy * iPositionScale.z + iPositionScale.xy, position.zw);
            }
        );
        
        static const std::string cpFrag = OF_GLSL(150,
            in vec2 vTexCoord;
            in vec4 vColor;
            
            out vec4 fragColor;
              
            void main(void)
            {
                vec2 uv = vTexCoord * 2.0 - 1.0;
                float d = dot(uv, uv);
                float rim = smoothstep(0.7, 0.8, d);
                rim += smoothstep(0.3, 0.4, d) - smoothstep(0.5, 0.6, d);
                rim += smoothstep(0.1, 0.0, d);
                fragColor = mix(vec4( 0.0, 0.0, 0.0, 0.25), vColor, rim);
            }
        );
        
        // Load the shader.
        controlShader.setupShaderFromSource(GL_VERTEX_SHADER, cpVert);
        controlShader.setupShaderFromSource(GL_FRAGMENT_SHADER, cpFrag);
        controlShader.bindAttribute(INSTANCE_POS_SCALE_ATTRIBUTE, "iPositionScale");
        controlShader.bindAttribute(INSTANCE_COLOR_ATTRIBUTE, "iColor");
        controlShader.bindDefaults();
        controlShader.linkProgram();
    }
}

//--------------------------------------------------------------
void RemoteWarpBase::drawControlPoints()
{
    setupControlPoints();
    
    if (!controlData.empty())
    {
        controlMesh.getVbo().updateAttributeData(INSTANCE_POS_SCALE_ATTRIBUTE, (float *)&controlData[0].pos, controlData.size());
        controlMesh.getVbo().updateAttributeData(INSTANCE_COLOR_ATTRIBUTE, (float *)&controlData[0].color, controlData.size());
        
        controlShader.begin();
        {
            controlMesh.drawInstanced(OF_MESH_FILL, controlData.size());
        }
        controlShader.end();
    }
    
    controlData.clear();
}

//--------------------------------------------------------------
bool RemoteWarpBase::handleCursorDown(const glm::vec2 & pos)
{
    if (!editing || selectedIndex >= controlPoints.size()) return false;
    
    // Calculate offset by converting control point from normalized to screen space.
    glm::vec2 screenPoint = (getControlPoint(selectedIndex) * windowSize);
    selectedOffset = pos - screenPoint;
    
    return true;
}

//--------------------------------------------------------------
bool RemoteWarpBase::handleCursorDrag(const glm::vec2 & pos)
{
    if (!editing || selectedIndex >= controlPoints.size()) return false;
    
    // Set control point in normalized space.
    glm::vec2 screenPoint = pos - selectedOffset;
    setControlPoint(selectedIndex, screenPoint / windowSize);
    
    dirty = true;
    
    return true;
}

//--------------------------------------------------------------
bool RemoteWarpBase::handleWindowResize(int width, int height)
{
    auto newTL = glm::vec2(drawArea.getTopLeft()) / windowSize;
    auto newBR = glm::vec2(drawArea.getBottomRight()) / windowSize;
    windowSize = glm::vec2(width, height);
    newTL *= windowSize;
    newBR *= windowSize;
    drawArea = ofRectangle(newTL,newBR);
    dirty = true;
    return true;
}
