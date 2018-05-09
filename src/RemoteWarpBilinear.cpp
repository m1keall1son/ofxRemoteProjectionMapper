//
//  RemoteWarpBilinear.cpp
//  RemoteProjectionMapper
//
//  Created by Michael Allison on 5/1/18.
//

#include "RemoteWarpBilinear.h"
#include "ofMain.h"

static const std::string blVert = OF_GLSL(150,
      // OF default uniforms and attributes
      uniform mat4 modelViewProjectionMatrix;
      uniform vec4 globalColor;
      
      in vec4 position;
      in vec2 texcoord;
      in vec4 color;
      
      // App uniforms and attributes
      out vec2 vTexCoord;
      out vec4 vColor;
      
      void main(void){
            vTexCoord = texcoord;
            vColor = globalColor;
            
            gl_Position = modelViewProjectionMatrix * position;
        }
    );
static const std::string blFrag = OF_GLSL(150,
      uniform sampler2D uTexture;
      uniform vec4 uExtends;
      uniform vec3 uLuminance;
      uniform vec3 uGamma;
      uniform vec4 uEdges;
      uniform vec4 uCorners;
      uniform float uExponent;
      uniform bool uEditing;
      
    in vec2 vTexCoord;
    in vec4 vColor;
      
    out vec4 fragColor;
      
    float map(in float value, in float inMin, in float inMax, in float outMin, in float outMax)
    {
            return outMin + (outMax - outMin) * (value - inMin) / (inMax - inMin);
    }
                                          
    float grid(in vec2 uv, in vec2 size)
    {
        vec2 coord = uv / size;
        vec2 grid = abs(fract(coord - 0.5) - 0.5) / (2.0 * fwidth(coord));
        float line = min(grid.x, grid.y);
        return 1.0 - min(line, 1.0);
    }
                                          
    void main(void)
    {
        vec4 texColor = texture(uTexture, vTexCoord);
        
        vec2 mapCoord = vec2(map(vTexCoord.x, uCorners.x, uCorners.z, 0.0, 1.0), map(vTexCoord.y, uCorners.y, uCorners.w, 0.0, 1.0));
        
        float a = 1.0;
        if (uEdges.x > 0.0) a *= clamp(mapCoord.x / uEdges.x, 0.0, 1.0);
        if (uEdges.y > 0.0) a *= clamp(mapCoord.y / uEdges.y, 0.0, 1.0);
        if (uEdges.z > 0.0) a *= clamp((1.0 - mapCoord.x) / uEdges.z, 0.0, 1.0);
        if (uEdges.w > 0.0) a *= clamp((1.0 - mapCoord.y) / uEdges.w, 0.0, 1.0);
        
        const vec3 one = vec3(1.0);
        vec3 blend = (a < 0.5) ? (uLuminance * pow(2.0 * a, uExponent)) : one - (one - uLuminance) * pow(2.0 * (1.0 - a), uExponent);
        
        texColor.rgb *= pow(blend, one / uGamma);
        
        if (uEditing)
        {
            float f = grid(mapCoord.xy * uExtends.xy, uExtends.zw);
            vec4 gridColor = vec4(1.0f);
            fragColor = mix(texColor * vColor, gridColor, f);
        }
        else
        {
            fragColor = texColor * vColor;
        }
    }
    );

RemoteWarpBilinear::RemoteWarpBilinear(const std::string& name, const WarpSettings& settings) :
    RemoteWarpBase(name, settings.type(WarpSettings::TYPE_BILINEAR)),
    linear(false),
    adaptive(true),
    corners(0.0f, 0.0f, 1.0f, 1.0f),
    resolutionX(0),
    resolutionY(0),
    resolution(16),  // higher value is coarser mesh
    remoteNumControlsX(2),
    remoteNumControlsY(2)
{
    if(!this->shader.isLoaded()){
        this->shader.setupShaderFromSource(GL_VERTEX_SHADER, blVert);
        this->shader.setupShaderFromSource(GL_FRAGMENT_SHADER, blFrag);
        this->shader.bindDefaults();
        this->shader.linkProgram();
    }
    
    RUI_SHARE_PARAM_WCN(warpName+"-adaptive",adaptive);
    RUI_SHARE_PARAM_WCN(warpName+"-linear",linear);
    RUI_SHARE_PARAM_WCN(warpName+"-numControlsX",remoteNumControlsX,2,10);
    RUI_SHARE_PARAM_WCN(warpName+"-numControlsY",remoteNumControlsY,2,10);
    RUI_SHARE_PARAM_WCN(warpName+"-incResolution",remoteIncRes);
    RUI_SHARE_PARAM_WCN(warpName+"-decResolution",remoteDecRes);
    RUI_SHARE_PARAM_WCN(warpName+"-resolution",resolution,16,128);
    RUI_SHARE_PARAM_WCN(warpName+"-flipVertical",remoteFlipV);
    RUI_SHARE_PARAM_WCN(warpName+"-flipHorizontal",remoteFlipH);
    
    if(std::filesystem::exists(saveLocation/currentPreset/RemoteWarpBase::sSaveFilename)){
        loadControlPoints(saveLocation/currentPreset/RemoteWarpBase::sSaveFilename);
        dirty = true;
    }else{
        reset();
    }
}

RemoteWarpBilinear::~RemoteWarpBilinear()
{
}

void RemoteWarpBilinear::handleRemoteUpdate(RemoteUIServerCallBackArg & arg)
{    
    switch (arg.action) {
        case CLIENT_UPDATED_PARAM:
            if(arg.group == remoteGroupName){
                if(arg.paramName == (warpName+"-adaptive") || arg.paramName == (warpName+"-linear") ||  arg.paramName == (warpName+"-resolution")){
                    dirty = true;
                }else if(arg.paramName == (warpName+"-numControlsX")){
                    setNumControlsX(remoteNumControlsX);
                }else if(arg.paramName == (warpName+"-numControlsY")){
                    setNumControlsY(remoteNumControlsY);
                }else if(arg.paramName == (warpName+"-incResolution")){
                    increaseResolution();
                    remoteIncRes = false;
                    RUI_PUSH_TO_CLIENT();
                }else if(arg.paramName == (warpName+"-decResolution")){
                    decreaseResolution();
                    remoteDecRes = false;
                    RUI_PUSH_TO_CLIENT();
                }else if(arg.paramName == (warpName+"-flipHorizontal")){
                    flipHorizontal();
                    remoteFlipH = false;
                    RUI_PUSH_TO_CLIENT();
                }else if(arg.paramName == (warpName+"-flipVertical")){
                    flipVertical();
                    remoteFlipV = false;
                    RUI_PUSH_TO_CLIENT();
                }else if( arg.paramName.substr(0,ctrlptPrefix.size()) == ctrlptPrefix ){
                    dirty = true;
                }
            }
            break;
        default:
            break;
    }
    RemoteWarpBase::handleRemoteUpdate(arg);
}

//--------------------------------------------------------------
void RemoteWarpBilinear::serialize(nlohmann::json & json)
{
    RemoteWarpBase::serialize(json);
    
    json["resolution"] = this->resolution;
    json["linear"] = this->linear;
    json["adaptive"] = this->adaptive;
}

//--------------------------------------------------------------
void RemoteWarpBilinear::deserialize(const nlohmann::json & json)
{
    RemoteWarpBase::deserialize(json);
    
    this->resolution = json["resolution"];
    this->linear = json["linear"];
    this->adaptive = json["adaptive"];
}

//--------------------------------------------------------------
void RemoteWarpBilinear::setLinear(bool linear)
{
    this->linear = linear;
    this->dirty = true;
}

//--------------------------------------------------------------
bool RemoteWarpBilinear::getLinear() const
{
    return this->linear;
}

//--------------------------------------------------------------
void RemoteWarpBilinear::setAdaptive(bool adaptive)
{
    this->adaptive = adaptive;
    this->dirty = true;
}

//--------------------------------------------------------------
bool RemoteWarpBilinear::getAdaptive() const
{
    return this->adaptive;
}

//--------------------------------------------------------------
void RemoteWarpBilinear::increaseResolution()
{
    if (this->resolution < 64)
    {
        this->resolution += 4;
        this->dirty = true;
    }
}

//--------------------------------------------------------------
void RemoteWarpBilinear::decreaseResolution()
{
    if (this->resolution > 4)
    {
        this->resolution -= 4;
        this->dirty = true;
    }
}

//--------------------------------------------------------------
int RemoteWarpBilinear::getResolution() const
{
    return this->resolution;
}

//--------------------------------------------------------------
void RemoteWarpBilinear::reset(const glm::vec2 & scale, const glm::vec2 & offset)
{
    this->controlPoints.clear();
    for (auto x = 0; x < this->numControlsX; ++x)
    {
        for (auto y = 0; y < this->numControlsY; ++y)
        {
            this->controlPoints.push_back(glm::vec2(x / float(this->numControlsX - 1), y / float(this->numControlsY - 1)) * scale + offset);
        }
    }
    
    this->dirty = true;
}

//--------------------------------------------------------------
void RemoteWarpBilinear::drawTexture(const ofTexture & texture, const ofRectangle & srcBounds, const ofRectangle & dstBounds)
{
    // Clip against bounds.
    auto srcClip = srcBounds;
    auto dstClip = dstBounds;
    this->clip(srcClip, dstClip);
    
    // Set corner texture coordinates.
    if (texture.getTextureData().textureTarget == GL_TEXTURE_RECTANGLE_ARB)
    {
        if (texture.getTextureData().bFlipTexture)
        {
            this->setCorners(srcClip.getMinX(), srcClip.getMaxY(), srcClip.getMaxX(), srcClip.getMinY());
        }
        else
        {
            this->setCorners(srcClip.getMinX(), srcClip.getMinY(), srcClip.getMaxX(), srcClip.getMaxY());
        }
    }
    else
    {
        if (texture.getTextureData().bFlipTexture)
        {
            this->setCorners(srcClip.getMinX() / texture.getWidth(), srcClip.getMaxY() / texture.getHeight(), srcClip.getMaxX() / texture.getWidth(), srcClip.getMinY() / texture.getHeight());
        }
        else
        {
            this->setCorners(srcClip.getMinX() / texture.getWidth(), srcClip.getMinY() / texture.getHeight(), srcClip.getMaxX() / texture.getWidth(), srcClip.getMaxY() / texture.getHeight());
        }
    }
    
    this->setupVbo();
    
    auto currentColor = ofGetStyle().color;
    ofPushStyle();
    {
        auto wasDepthTest = glIsEnabled(GL_DEPTH_TEST);
        ofDisableDepthTest();
        
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        
        // Adjust brightness.
        if (this->brightness < 1.0f)
        {
            currentColor *= this->brightness;
            ofSetColor(currentColor);
        }
        
        this->shader.begin();
        {
            this->shader.setUniformTexture("uTexture", texture, 1);
            this->shader.setUniform4f("uExtends", glm::vec4(this->width, this->height, this->width / float(this->numControlsX - 1), this->height / float(this->numControlsY - 1)));
            this->shader.setUniform3f("uLuminance", this->luminance);
            this->shader.setUniform3f("uGamma", this->gamma);
            this->shader.setUniform4f("uEdges", this->edges);
            this->shader.setUniform4f("uCorners", this->corners);
            this->shader.setUniform1f("uExponent", this->exponent);
            this->shader.setUniform1i("uEditing", this->editing);
            
            this->vbo.drawElements(GL_TRIANGLES, this->vbo.getNumIndices());
        }
        this->shader.end();
        
        if (wasDepthTest)
        {
            ofEnableDepthTest();
        }
    }
    ofPopStyle();
}

//--------------------------------------------------------------
void RemoteWarpBilinear::drawControls()
{
    if (this->editing)
    {
        // Draw control points.
        for (auto i = 0; i < this->controlPoints.size(); ++i)
        {
            auto found = std::find_if(selectedIndices.begin(), selectedIndices.end(),[i](const Selection& s){
                return s.index == i;
            });
            this->queueControlPoint(this->getControlPoint(i) * this->windowSize, found != selectedIndices.end());
        }
        
        this->drawControlPoints();
    }
}


//--------------------------------------------------------------
void RemoteWarpBilinear::setupVbo()
{
    if (this->dirty)
    {
        if (this->adaptive)
        {
            // Determine a suitable mesh resolution based on the dimensions of the window
            // and the size of the mesh in pixels.
            auto meshBounds = this->getMeshBounds();
            this->setupMesh(meshBounds.getWidth() / this->resolution, meshBounds.getHeight() / this->resolution);
        }
        else
        {
            // Use a fixed mesh resolution.
            this->setupMesh(this->width / this->resolution, this->height / this->resolution);
        }
        this->updateMesh();
    }
}

//--------------------------------------------------------------
void RemoteWarpBilinear::setupMesh(int resolutionX, int resolutionY)
{
    // Convert from number of quads to number of vertices.
    ++resolutionX;
    ++resolutionY;
    
    // Find a value for resolutionX and resolutionY that can be evenly divided by numControlsX and numControlsY.
    if (this->numControlsX < resolutionX)
    {
        int dx = (resolutionX - 1) % (this->numControlsX - 1);
        if (dx >= (this->numControlsX / 2))
        {
            dx -= (this->numControlsX - 1);
        }
        resolutionX -= dx;
    }
    else
    {
        resolutionX = this->numControlsX;
    }
    
    if (this->numControlsY < resolutionY)
    {
        int dy = (resolutionY - 1) % (this->numControlsY - 1);
        if (dy >= (this->numControlsY / 2))
        {
            dy -= (this->numControlsY - 1);
        }
        resolutionY -= dy;
    }
    else
    {
        resolutionY = this->numControlsY;
    }
    
    this->resolutionX = resolutionX;
    this->resolutionY = resolutionY;
    
    int numVertices = (resolutionX * resolutionY);
    int numTriangles = 2 * (resolutionX - 1) * (resolutionY - 1);
    int numIndices = numTriangles * 3;
    
    // Build the static data.
    int i = 0;
    int j = 0;
    
    auto indices = std::vector<ofIndexType>(numIndices);
    auto texCoords = std::vector<glm::vec2>(numVertices);
    
    for (int x = 0; x < resolutionX; ++x)
    {
        for (int y = 0; y < resolutionY; ++y)
        {
            // Index.
            if (((x + 1) < resolutionX) && ((y + 1) < resolutionY))
            {
                indices[i++] = (x + 0) * resolutionY + (y + 0);
                indices[i++] = (x + 1) * resolutionY + (y + 0);
                indices[i++] = (x + 1) * resolutionY + (y + 1);
                
                indices[i++] = (x + 0) * resolutionY + (y + 0);
                indices[i++] = (x + 1) * resolutionY + (y + 1);
                indices[i++] = (x + 0) * resolutionY + (y + 1);
            }
            
            // Tex Coord.
            float tx = ofLerp(this->corners.x, this->corners.z, x / (float)(this->resolutionX - 1));
            float ty = ofLerp(this->corners.y, this->corners.w, y / (float)(this->resolutionY - 1));
            
            texCoords[j++] = glm::vec2(tx, ty);
        }
    }
    
    // Build placeholder data.
    std::vector<glm::vec3> positions(this->resolutionX * this->resolutionY);
    
    // Build mesh.
    this->vbo.clear();
    this->vbo.setVertexData(positions.data(), positions.size(), GL_STATIC_DRAW);
    this->vbo.setTexCoordData(texCoords.data(), texCoords.size(), GL_STATIC_DRAW);
    this->vbo.setIndexData(indices.data(), indices.size(), GL_STATIC_DRAW);
    
    this->dirty = true;
}

// Mapped buffer seems to be a *tiny* bit faster.
#define USE_MAPPED_BUFFER 1

//--------------------------------------------------------------
void RemoteWarpBilinear::updateMesh()
{
    if (!this->vbo.getIsAllocated() || !this->dirty) return;
    
    glm::vec2 pt;
    float u, v;
    int col, row;
    
    std::vector<glm::vec2> cols, rows;
    
#if USE_MAPPED_BUFFER
    auto vertexBuffer = this->vbo.getVertexBuffer();
    auto mappedMesh = (glm::vec3 *)vertexBuffer.map(GL_WRITE_ONLY);
#else
    std::vector<glm::vec3> positions(this->resolutionX * this->resolutionY);
    auto index = 0;
#endif
    
    for (auto x = 0; x < this->resolutionX; ++x)
    {
        for (auto y = 0; y < this->resolutionY; ++y)
        {
            // Transform coordinates to [0..numControls]
            u = x * (this->numControlsX - 1) / (float)(this->resolutionX - 1);
            v = y * (this->numControlsY - 1) / (float)(this->resolutionY - 1);
            
            // Determine col and row.
            col = (int)u;
            row = (int)v;
            
            // Normalize coordinates to [0..1]
            u -= col;
            v -= row;
            
            if (this->linear)
            {
                // Perform linear interpolation.
                auto p1 = (1.0f - u) * this->getPoint(col, row) + u * this->getPoint(col + 1, row);
                auto p2 = (1.0f - u) * this->getPoint(col, row + 1) + u * this->getPoint(col + 1, row + 1);
                pt = ((1.0f - v) * p1 + v * p2) * this->windowSize;
            }
            else {
                // Perform bicubic interpolation.
                rows.clear();
                for (int i = -1; i < 3; ++i)
                {
                    cols.clear();
                    for (int j = -1; j < 3; ++j)
                    {
                        cols.push_back(this->getPoint(col + i, row + j));
                    }
                    rows.push_back(this->cubicInterpolate(cols, v));
                }
                pt = this->cubicInterpolate(rows, u) * this->windowSize;
            }
            
#if USE_MAPPED_BUFFER
            *mappedMesh++ = glm::vec3(pt.x, pt.y, 0.0f);
#else
            positions[index++] = glm::vec3(pt.x, pt.y, 0.0f);
#endif
        }
    }
    
#if USE_MAPPED_BUFFER
    vertexBuffer.unmap();
#else
    this->vbo.updateVertexData(positions.data(), positions.size());
#endif
    
    this->dirty = false;
}

//--------------------------------------------------------------
glm::vec2 RemoteWarpBilinear::getPoint(int col, int row) const
{
    auto maxCol = this->numControlsX - 1;
    auto maxRow = this->numControlsY - 1;
    
    // Here's the magic: extrapolate points beyond the edges.
    if (col < 0)
    {
        return (2.0f * getPoint(0, row) - getPoint(0 - col, row));
    }
    if (row < 0)
    {
        return (2.0f * getPoint(col, 0) - getPoint(col, 0 - row));
    }
    if (col > maxCol)
    {
        return (2.0f * getPoint(maxCol, row) - getPoint(2 * maxCol - col, row));
    }
    if (row > maxRow)
    {
        return (2.0f * getPoint(col, maxRow) - getPoint(col, 2 * maxRow - row));
    }
    
    // Points on the edges or within the mesh can simply be looked up.
    auto idx = (col * this->numControlsY) + row;
    return this->controlPoints[idx];
}

//--------------------------------------------------------------
// From http://www.paulinternet.nl/?page=bicubic : fast catmull-rom calculation
glm::vec2 RemoteWarpBilinear::cubicInterpolate(const std::vector<glm::vec2> & knots, float t) const
{
    assert(knots.size() >= 4);
    
    return (knots[1] + 0.5f * t * (knots[2] - knots[0] + t * (2.0f * knots[0] - 5.0f * knots[1] + 4.0f * knots[2] - knots[3] + t * (3.0f * (knots[1] - knots[2]) + knots[3] - knots[0]))));
}

//--------------------------------------------------------------
void RemoteWarpBilinear::setNumControlsX(int n)
{
    if(remoteEditMode){
        removeControlPoints();
    }
    
    // There should be a minimum of 2 control points.
    n = MAX(2, n);
    
    // Prevent overflow.
    if ((n * this->numControlsY) > MAX_NUM_CONTROL_POINTS) return;
    
    // Create a list of new points.
    std::vector<glm::vec2> tempPoints(n * this->numControlsY);
    
    // Perform spline fitting.
    for (auto row = 0; row < this->numControlsY; ++row) {
        if (this->linear)
        {
            // Construct piece-wise linear spline.
            ofPolyline polyline;
            for (auto col = 0; col < this->numControlsX; ++col)
            {
                polyline.lineTo(glm::vec3(this->getPoint(col, row), 0.0f));
            }
            
            // Calculate position of new control points.
            auto step = 1.0f / (n - 1);
            for (auto col = 0; col < n; ++col)
            {
                auto idx = (col * this->numControlsY) + row;
                tempPoints[idx] = glm::vec2(polyline.getPointAtPercent(col * step));
            }
        }
        else
        {
            // Construct piece-wise catmull-rom spline.
            ofPolyline polyline;
            for (auto col = 0; col < this->numControlsX; ++col)
            {
                auto p0 = this->getPoint(col - 1, row);
                auto p1 = this->getPoint(col, row);
                auto p2 = this->getPoint(col + 1, row);
                auto p3 = this->getPoint(col + 2, row);
                
                // Control points according to an optimized Catmull-Rom implementation
                auto b1 = p1 + (p2 - p0) / 6.0f;
                auto b2 = p2 - (p3 - p1) / 6.0f;
                
                if (col == 0)
                {
                    polyline.lineTo(glm::vec3(p1, 0.0f));
                }
                
                polyline.curveTo(glm::vec3(p1, 0.0f));
                
                if (col < (this->numControlsX - 1))
                {
                    polyline.curveTo(glm::vec3(b1, 0.0f));
                    polyline.curveTo(glm::vec3(b2, 0.0f));
                }
                else
                {
                    polyline.lineTo(glm::vec3(p1, 0.0f));
                }
            }
            
            // Calculate position of new control points.
            auto step = 1.0f / (n - 1);
            for (auto col = 0; col < n; ++col)
            {
                auto idx = (col * this->numControlsY) + row;
                tempPoints[idx] = glm::vec2(polyline.getPointAtPercent(col * step));
            }
        }
    }
    
    // Save new control points.
    this->controlPoints = tempPoints;
    this->numControlsX = n;
    
    // Find new closest control point.
    float distance;
    //this->selectedIndex = this->findClosestControlPoint(glm::vec2(ofGetMouseX(), ofGetMouseY()), &distance);
    
    this->dirty = true;
    
    if(remoteEditMode){
        addControlPoints();
    }
}

//--------------------------------------------------------------
void RemoteWarpBilinear::setNumControlsY(int n)
{
    if(remoteEditMode){
        removeControlPoints();
    }
    
    // There should be a minimum of 2 control points.
    n = MAX(2, n);
    
    // Prevent overflow.
    if ((this->numControlsX * n) > MAX_NUM_CONTROL_POINTS) return;
    
    // Create a list of new points.
    std::vector<glm::vec2> tempPoints(this->numControlsX * n);
    
    // Perform spline fitting
    for (auto col = 0; col < this->numControlsX; ++col)
    {
        if (this->linear)
        {
            // Construct piece-wise linear spline.
            ofPolyline polyline;
            for (auto row = 0; row < this->numControlsY; ++row)
            {
                polyline.lineTo(glm::vec3(this->getPoint(col, row), 0.0f));
            }
            
            // Calculate position of new control points.
            float step = 1.0f / (n - 1);
            for (auto row = 0; row < n; ++row)
            {
                auto idx = (col * n) + row;
                tempPoints[idx] = glm::vec2(polyline.getPointAtPercent(row * step));
            }
        }
        else
        {
            // Construct piece-wise catmull-rom spline.
            ofPolyline polyline;
            for (auto row = 0; row < this->numControlsY; ++row)
            {
                auto p0 = this->getPoint(col, row - 1);
                auto p1 = this->getPoint(col, row);
                auto p2 = this->getPoint(col, row + 1);
                auto p3 = this->getPoint(col, row + 2);
                
                // Control points according to an optimized Catmull-Rom implementation
                auto b1 = p1 + (p2 - p0) / 6.0f;
                auto b2 = p2 - (p3 - p1) / 6.0f;
                
                if (row == 0)
                {
                    polyline.lineTo(glm::vec3(p1, 0.0f));
                }
                
                polyline.curveTo(glm::vec3(p1, 0.0f));
                
                if (row < (this->numControlsY - 1))
                {
                    polyline.curveTo(glm::vec3(b1, 0.0f));
                    polyline.curveTo(glm::vec3(b2, 0.0f));
                }
                else
                {
                    polyline.lineTo(glm::vec3(p1, 0.0f));
                }
            }
            
            // Calculate position of new control points.
            auto step = 1.0f / (n - 1);
            for (auto row = 0; row < n; ++row)
            {
                auto idx = (col * n) + row;
                tempPoints[idx] = glm::vec2(polyline.getPointAtPercent(row * step));
            }
        }
    }
    
    // Save new control points.
    this->controlPoints = tempPoints;
    this->numControlsY = n;
    
    // Find new closest control point.
    float distance;
    //this->selectedIndex = this->findClosestControlPoint(glm::vec2(ofGetMouseX(), ofGetMouseY()), &distance);
    
    this->dirty = true;
    
    if(remoteEditMode){
        addControlPoints();
    }
}

//--------------------------------------------------------------
ofRectangle RemoteWarpBilinear::getMeshBounds() const
{
    auto min = glm::vec2(1.0f);
    auto max = glm::vec2(0.0f);
    
    for (auto & pt : this->controlPoints)
    {
        min.x = MIN(pt.x, min.x);
        min.y = MIN(pt.y, min.y);
        max.x = MAX(pt.x, max.x);
        max.y = MAX(pt.y, min.y);
    }
    
    return ofRectangle(min * this->windowSize, max * this->windowSize);
}

//--------------------------------------------------------------
void RemoteWarpBilinear::setCorners(float left, float top, float right, float bottom)
{
    this->dirty |= (left != this->corners.x || top != this->corners.y || right != this->corners.z || bottom != this->corners.w);
    if (!this->dirty) return;
    
    this->corners = glm::vec4(left, top, right, bottom);
}

//--------------------------------------------------------------
void RemoteWarpBilinear::rotateClockwise()
{
    ofLogWarning("WarpBilinear::rotateClockwise") << "Not implemented!";
}

//--------------------------------------------------------------
void RemoteWarpBilinear::rotateCounterclockwise()
{
    ofLogWarning("WarpBilinear::rotateCounterclockwise") << "Not implemented!";
}

//--------------------------------------------------------------
void RemoteWarpBilinear::flipHorizontal()
{
    std::vector<glm::vec2> flippedPoints;
    for (int x = this->numControlsX - 1; x >= 0; --x)
    {
        for (int y = 0; y < this->numControlsY; ++y)
        {
            auto i = (x * this->numControlsY + y);
            flippedPoints.push_back(this->controlPoints[i]);
        }
    }
    this->controlPoints = flippedPoints;
    this->dirty = true;
    
    // Find new closest control point.
    float distance;
    //this->selectedIndex = this->findClosestControlPoint(glm::vec2(ofGetMouseX(), ofGetMouseY()), &distance);
}

//--------------------------------------------------------------
void RemoteWarpBilinear::flipVertical()
{
    std::vector<glm::vec2> flippedPoints;
    for (int x = 0; x < this->numControlsX; ++x)
    {
        for (int y = this->numControlsY - 1; y >= 0; --y)
        {
            auto i = (x * this->numControlsY + y);
            flippedPoints.push_back(this->controlPoints[i]);
        }
    }
    this->controlPoints = flippedPoints;
    this->dirty = true;
    
    // Find new closest control point.
    float distance;
    //this->selectedIndex = this->findClosestControlPoint(glm::vec2(ofGetMouseX(), ofGetMouseY()), &distance);
}

