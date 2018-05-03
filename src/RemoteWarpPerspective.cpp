//
//  RemoteWarpPerspective.cpp
//  RemoteProjectionMapper
//
//  Created by Michael Allison on 5/1/18.
//

#include "RemoteWarpPerspective.h"

static const std::string prVert = OF_GLSL(150,
      // OF default uniforms and attributes
      uniform mat4 modelViewProjectionMatrix;
      uniform vec4 globalColor;
      
      in vec4 position;
      in vec2 texcoord;
      in vec4 color;
      
      // App uniforms and attributes
      out vec2 vTexCoord;
      out vec4 vColor;
      
      void main(void)
        {
            vTexCoord = texcoord;
            vColor = globalColor;
            
            gl_Position = modelViewProjectionMatrix * position;
        }
    );
static const std::string prFrag = OF_GLSL(150,
      uniform sampler2D uTexture;
      uniform vec3 uLuminance;
      uniform vec3 uGamma;
      uniform vec4 uEdges;
      uniform vec4 uCorners;
      uniform float uExponent;
      
      in vec2 vTexCoord;
      in vec4 vColor;
      
      out vec4 fragColor;
      
      float map(in float value, in float inMin, in float inMax, in float outMin, in float outMax)
    {
        return outMin + (outMax - outMin) * (value - inMin) / (inMax - inMin);
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
        
        fragColor = texColor * vColor;
    }
    );

//--------------------------------------------------------------
RemoteWarpPerspective::RemoteWarpPerspective(const std::string& name, const WarpSettings& settings):
    RemoteWarpBase(name, settings.type(WarpSettings::TYPE_PERSPECTIVE))
{
    
    this->srcPoints[0] = glm::vec2(0.0f, 0.0f);
    this->srcPoints[1] = glm::vec2(this->width, 0.0f);
    this->srcPoints[2] = glm::vec2(this->width, this->height);
    this->srcPoints[3] = glm::vec2(0.0f, this->height);

    if(!this->shader.isLoaded()){
        this->shader.setupShaderFromSource(GL_VERTEX_SHADER, prVert);
        this->shader.setupShaderFromSource(GL_FRAGMENT_SHADER, prFrag);
        this->shader.bindDefaults();
        this->shader.linkProgram();
    }
    //this->shader.setupShaderFromFile(GL_VERTEX_SHADER, ofToDataPath("shaders/ofxWarp/WarpPerspective.vert"));
    //this->shader.setupShaderFromFile(GL_FRAGMENT_SHADER, ofToDataPath("shaders/ofxWarp/WarpPerspective.frag"));
    
    RUI_SHARE_PARAM_WCN(warpName+"-flipVertical",remoteFlipV);
    RUI_SHARE_PARAM_WCN(warpName+"-flipHorizontal",remoteFlipH);
    RUI_SHARE_PARAM_WCN(warpName+"-rot CW",remoteRotateCW);
    RUI_SHARE_PARAM_WCN(warpName+"-rot CCW",remoteRotateCCW);
    
    if(std::filesystem::exists(saveLocation/currentPreset/RemoteWarpBase::sSaveFilename)){
        loadControlPoints(saveLocation/currentPreset/RemoteWarpBase::sSaveFilename);
        dirty = true;
    }else{
        reset();
    }
    
}

void RemoteWarpPerspective::handleRemoteUpdate(RemoteUIServerCallBackArg & arg)
{    
    switch (arg.action) {
        case CLIENT_UPDATED_PARAM:
            if(arg.group == remoteGroupName){
                if(arg.paramName == (warpName+"-flipHorizontal")){
                    flipHorizontal();
                    remoteFlipH = false;
                    RUI_PUSH_TO_CLIENT();
                }else if(arg.paramName == (warpName+"-flipVertical")){
                    flipVertical();
                    remoteFlipV = false;
                    RUI_PUSH_TO_CLIENT();
                }else if(arg.paramName == (warpName+"-rot CW")){
                    rotateClockwise();
                    remoteRotateCW = false;
                    RUI_PUSH_TO_CLIENT();
                }else if(arg.paramName == (warpName+"-rot CCW")){
                    rotateCounterclockwise();
                    remoteRotateCCW = false;
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
RemoteWarpPerspective::~RemoteWarpPerspective()
{
    
}

//--------------------------------------------------------------
const glm::mat4 & RemoteWarpPerspective::getTransform()
{
    // Calculate warp matrix.
    if (this->dirty) {
        // Update source size.
        this->srcPoints[1].x = this->width;
        this->srcPoints[2].x = this->width;
        this->srcPoints[2].y = this->height;
        this->srcPoints[3].y = this->height;
        
        // Convert corners to actual destination pixels.
        for (int i = 0; i < 4; ++i)
        {
            this->dstPoints[i] = this->controlPoints[i] * this->windowSize;
        }
        
        // Calculate warp matrix.
        this->transform = PerspectiveTransformation::transform(this->srcPoints, this->dstPoints);
        this->transformInverted = glm::inverse(this->transform);
        
        this->dirty = false;
    }
    
    return this->transform;
}

//--------------------------------------------------------------
const glm::mat4 & RemoteWarpPerspective::getTransformInverted()
{
    if (this->dirty)
    {
        this->getTransform();
    }
    
    return this->transformInverted;
}

//--------------------------------------------------------------
void RemoteWarpPerspective::reset(const glm::vec2 & scale, const glm::vec2 & offset)
{
    this->controlPoints.clear();
    
    this->controlPoints.push_back(glm::vec2(0.0f, 0.0f) * scale + offset);
    this->controlPoints.push_back(glm::vec2(1.0f, 0.0f) * scale + offset);
    this->controlPoints.push_back(glm::vec2(1.0f, 1.0f) * scale + offset);
    this->controlPoints.push_back(glm::vec2(0.0f, 1.0f) * scale + offset);
    
    this->dirty = true;
}

//--------------------------------------------------------------
void RemoteWarpPerspective::begin()
{
    ofPushMatrix();
    ofMultMatrix(this->getTransform());
}

//--------------------------------------------------------------
void RemoteWarpPerspective::end()
{
    ofPopMatrix();
    
    this->drawControls();
}

//--------------------------------------------------------------
void RemoteWarpPerspective::drawTexture(const ofTexture & texture, const ofRectangle & srcBounds, const ofRectangle & dstBounds)
{
    // Clip against bounds.
    auto srcClip = srcBounds;
    auto dstClip = dstBounds;
    this->clip(srcClip, dstClip);
    
    // Set corner texture coordinates.
    glm::vec4 corners;
    if (texture.getTextureData().textureTarget == GL_TEXTURE_RECTANGLE_ARB)
    {
        if (texture.getTextureData().bFlipTexture)
        {
            corners = glm::vec4(srcClip.getMinX(), srcClip.getMaxY(), srcClip.getMaxX(), srcClip.getMinY());
        }
        else
        {
            corners = glm::vec4(srcClip.getMinX(), srcClip.getMinY(), srcClip.getMaxX(), srcClip.getMaxY());
        }
    }
    else
    {
        if (texture.getTextureData().bFlipTexture)
        {
            corners = glm::vec4(srcClip.getMinX() / texture.getWidth(), srcClip.getMaxY() / texture.getHeight(), srcClip.getMaxX() / texture.getWidth(), srcClip.getMinY() / texture.getHeight());
        }
        else
        {
            corners = glm::vec4(srcClip.getMinX() / texture.getWidth(), srcClip.getMinY() / texture.getHeight(), srcClip.getMaxX() / texture.getWidth(), srcClip.getMaxY() / texture.getHeight());
        }
    }
    
    ofPushMatrix();
    {
        ofMultMatrix(this->getTransform());
        
        auto currentColor = ofGetStyle().color;
        ofPushStyle();
        {
            // Adjust brightness.
            if (this->brightness < 1.0f)
            {
                currentColor *= this->brightness;
                ofSetColor(currentColor);
            }
            
            // Draw texture.
            this->shader.begin();
            {
                this->shader.setUniformTexture("uTexture", texture, 1);
                this->shader.setUniform3f("uLuminance", this->luminance);
                this->shader.setUniform3f("uGamma", this->gamma);
                this->shader.setUniform4f("uEdges", this->edges);
                this->shader.setUniform4f("uCorners", corners);
                this->shader.setUniform1f("uExponent", this->exponent);
                
                const auto mesh = texture.getMeshForSubsection(dstClip.x, dstClip.y, 0.0f, dstClip.width, dstClip.height, srcClip.x, srcClip.y, srcClip.width, srcClip.height, ofIsVFlipped(), OF_RECTMODE_CORNER);
                mesh.draw();
            }
            this->shader.end();
        }
        ofPopStyle();
        
        if (this->editing)
        {
            // Draw grid lines.
            ofPushStyle();
            {
                glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
                
                ofSetColor(ofColor::white);
                
                for (int i = 0; i <= 1; ++i)
                {
                    float s = i / 1.0f;
                    ofDrawLine(s * this->width, 0.0f, s * this->width, this->height);
                    ofDrawLine(0.0f, s * this->height, this->width, s * this->height);
                }
                
                ofDrawLine(0.0f, 0.0f, this->width, this->height);
                ofDrawLine(this->width, 0.0f, 0.0f, this->height);
            }
            ofPopStyle();
        }
    }
    ofPopMatrix();
}

//--------------------------------------------------------------
void RemoteWarpPerspective::drawControls()
{
    if (this->editing && this->selectedIndex < this->controlPoints.size())
    {
        // Draw control points.
        for (auto i = 0; i < 4; ++i)
        {
            this->queueControlPoint(dstPoints[i], i == this->selectedIndex);
        }
        
        this->drawControlPoints();
    }
}

//--------------------------------------------------------------
void RemoteWarpPerspective::rotateClockwise()
{
    std::swap(this->controlPoints[3], this->controlPoints[0]);
    std::swap(this->controlPoints[0], this->controlPoints[1]);
    std::swap(this->controlPoints[1], this->controlPoints[2]);
    this->selectedIndex = (this->selectedIndex + 3) % 4;
    this->dirty = true;
}

//--------------------------------------------------------------
void RemoteWarpPerspective::rotateCounterclockwise()
{
    std::swap(this->controlPoints[1], this->controlPoints[2]);
    std::swap(this->controlPoints[0], this->controlPoints[1]);
    std::swap(this->controlPoints[3], this->controlPoints[0]);
    this->selectedIndex = (this->selectedIndex + 1) % 4;
    this->dirty = true;
}

//--------------------------------------------------------------
void RemoteWarpPerspective::flipHorizontal()
{
    std::swap(this->controlPoints[0], this->controlPoints[1]);
    std::swap(this->controlPoints[2], this->controlPoints[3]);
    if (this->selectedIndex % 2)
    {
        --this->selectedIndex;
    }
    else
    {
        ++this->selectedIndex;
    }
    this->dirty = true;
}

//--------------------------------------------------------------
void RemoteWarpPerspective::flipVertical()
{
    std::swap(this->controlPoints[0], this->controlPoints[3]);
    std::swap(this->controlPoints[1], this->controlPoints[2]);
    this->selectedIndex = (this->controlPoints.size() - 1) - this->selectedIndex;
    this->dirty = true;
}

glm::mat4 PerspectiveTransformation::transform(const glm::vec2 src[4], const glm::vec2 dst[4])
{
    float p[8][9] =
    {
        { -src[0][0], -src[0][1], -1, 0, 0, 0, src[0][0] * dst[0][0], src[0][1] * dst[0][0], -dst[0][0] }, // h11
        { 0, 0, 0, -src[0][0], -src[0][1], -1, src[0][0] * dst[0][1], src[0][1] * dst[0][1], -dst[0][1] }, // h12
        { -src[1][0], -src[1][1], -1, 0, 0, 0, src[1][0] * dst[1][0], src[1][1] * dst[1][0], -dst[1][0] }, // h13
        { 0, 0, 0, -src[1][0], -src[1][1], -1, src[1][0] * dst[1][1], src[1][1] * dst[1][1], -dst[1][1] }, // h21
        { -src[2][0], -src[2][1], -1, 0, 0, 0, src[2][0] * dst[2][0], src[2][1] * dst[2][0], -dst[2][0] }, // h22
        { 0, 0, 0, -src[2][0], -src[2][1], -1, src[2][0] * dst[2][1], src[2][1] * dst[2][1], -dst[2][1] }, // h23
        { -src[3][0], -src[3][1], -1, 0, 0, 0, src[3][0] * dst[3][0], src[3][1] * dst[3][0], -dst[3][0] }, // h31
        { 0, 0, 0, -src[3][0], -src[3][1], -1, src[3][0] * dst[3][1], src[3][1] * dst[3][1], -dst[3][1] }, // h32
    };
    
    PerspectiveTransformation::gaussianElimination(&p[0][0], 9);
    
    return glm::mat4(p[0][8], p[3][8], 0, p[6][8],
                     p[1][8], p[4][8], 0, p[7][8],
                     0, 0, 1, 0,
                     p[2][8], p[5][8], 0, 1);
}

void PerspectiveTransformation::gaussianElimination(float * input, int n)
{
    auto i = 0;
    auto j = 0;
    auto m = n - 1;
    
    while (i < m && j < n)
    {
        auto iMax = i;
        for (auto k = i + 1; k < m; ++k)
        {
            if (fabs(input[k * n + j]) > fabs(input[iMax * n + j]))
            {
                iMax = k;
            }
        }
        
        if (input[iMax * n + j] != 0)
        {
            if (i != iMax)
            {
                for (auto k = 0; k < n; ++k)
                {
                    auto ikIn = input[i * n + k];
                    input[i * n + k] = input[iMax * n + k];
                    input[iMax * n + k] = ikIn;
                }
            }
            
            float ijIn = input[i * n + j];
            for (auto k = 0; k < n; ++k)
            {
                input[i * n + k] /= ijIn;
            }
            
            for (auto u = i + 1; u < m; ++u)
            {
                auto ujIn = input[u * n + j];
                for (auto k = 0; k < n; ++k)
                {
                    input[u * n + k] -= ujIn * input[i * n + k];
                }
            }
            
            ++i;
        }
        ++j;
    }
    
    for (auto i = m - 2; i >= 0; --i)
    {
        for (auto j = i + 1; j < n - 1; ++j)
        {
            input[i * n + m] -= input[i * n + j] * input[j * n + m];
        }
    }
}

