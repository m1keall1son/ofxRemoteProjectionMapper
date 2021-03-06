//
//  RemoteWarpBase.hpp
//  RemoteProjectionMapper
//
//  Created by Michael Allison on 5/1/18.
//

#pragma once

#include "ofxRemoteUIServer.h"
#include <list>

#define OF_GLSL(vers, code) "#version "#vers"\n "#code

struct WarpSettings {
    
    typedef enum
    {
        TYPE_UNKNOWN,
        TYPE_BILINEAR,
        TYPE_PERSPECTIVE,
        TYPE_PERSPECTIVE_BILINEAR
    } Type;
    
    WarpSettings(Type type = Type::TYPE_UNKNOWN ):
    _type(type),
    _brightness(1),
    _luminance(.5),
    _gamma(1),
    _exponent(2),
    _edges(0),
    _saveLocation(ofToDataPath("mappings")),
    _srcArea(ofRectangle(0,0,640,480)),
    _width(640),
    _height(480),
    _drawArea(ofRectangle(0,0,ofGetWidth(), ofGetHeight()))
    {}
    
    WarpSettings& srcSize(int width, int height){ _width = width; _height = height; return *this; }
    WarpSettings& srcArea(const ofRectangle& area){ _srcArea = area; return *this; }
    WarpSettings& drawArea(const ofRectangle& area){ _drawArea = area; return *this; }

    WarpSettings& edges(float left, float top, float right, float bottom){ _edges = glm::vec4( left, top, right, bottom ); return *this; }
    WarpSettings& luminance(float r, float g, float b){ _luminance = glm::vec3(r,g,b); return *this;}
    WarpSettings& luminance(float val){ _luminance = glm::vec3(val); return *this;}
    WarpSettings& gamma(float r, float g, float b){ _gamma = glm::vec3(r,g,b); return *this; }
    WarpSettings& gamma(float val){ _gamma = glm::vec3(val); return *this; }
    WarpSettings& brightness(float val){ _brightness = val; return *this; }
    WarpSettings& exponent(float val){ _exponent = val; return *this; }
    WarpSettings& saveLocation(const std::filesystem::path& file){ _saveLocation = file; return *this; }

    const WarpSettings& type(Type type) const { _type = type; return *this; }

    mutable Type _type;
    float _brightness;
    glm::vec3 _luminance;
    glm::vec3 _gamma;
    float _exponent;
    glm::vec4 _edges;
    std::filesystem::path _saveLocation;
    ofRectangle _srcArea;
    ofRectangle _drawArea;
    int _width;
    int _height;
};

class RemoteWarpBase {
public:
    
    RemoteWarpBase(const std::string& name, const WarpSettings& settings);
    virtual ~RemoteWarpBase();
    
    inline const std::string& getName()const{return warpName;}
    inline const ofRectangle& getSrcArea()const{return srcArea;}
    inline const ofRectangle& getDrawArea()const{return drawArea;}
    inline glm::ivec2 getSrcSize()const{ return glm::ivec2(width,height); }

    //draw texture to warpped mapping
    virtual void drawWarp(const ofTexture& tex);
    
    //! returns the type of the warp
    WarpSettings::Type getType() const;
    
    virtual void serialize(nlohmann::json & json);
    virtual void deserialize(const nlohmann::json & json);
    
    virtual void setEditing(bool editing);
    void toggleEditing();
    bool isEditing() const;
    
    //! set the width of the content in pixels
    virtual void setWidth(float width);
    //! get the width of the content in pixels
    float getWidth() const;
    
    //! set the height of the content in pixels
    virtual void setHeight(float height);
    //! get the height of the content in pixels
    float getHeight() const;
    
    //! set the width and height of the content in pixels
    virtual void setSize(float width, float height);
    //! set the width and height of the content in pixels
    virtual void setSize(const glm::vec2 & size);
    //! get the width and height of the content in pixels
    glm::vec2 getSize() const;
    //! get the rectangle of the content in pixels
    ofRectangle getBounds() const;
    
    //! set the brightness value of the texture (values between 0 and 1)
    void setBrightness(float brightness);
    //! return the brightness value of the texture (values between 0 and 1)
    float getBrightness() const;
    
    //! set the luminance value for all color channels, used for edge blending (0.5 = linear)
    void setLuminance(float luminance);
    //! set the luminance value for the red, green and blue channels, used for edge blending (0.5 = linear)
    void setLuminance(float red, float green, float blue);
    //! set the luminance value for the red, green and blue channels, used for edge blending (0.5 = linear)
    void setLuminance(const glm::vec3 & rgb);
    //! returns the luminance value for the red, green and blue channels, used for edge blending (0.5 = linear)
    const glm::vec3 & getLuminance() const;
    
    //! set the gamma curve value for all color channels
    void setGamma(float gamma);
    //! set the gamma curve value for the red, green and blue channels
    void setGamma(float red, float green, float blue);
    //! set the gamma curve value for the red, green and blue channels
    void setGamma(const glm::vec3 & rgb);
    //! return the gamma curve value for the red, green and blue channels
    const glm::vec3 & getGamma() const;
    
    //! set the edge blending curve exponent  (1.0 = linear, 2.0 = quadratic)
    void setExponent(float exponent);
    //! return the edge blending curve exponent (1.0 = linear, 2.0 = quadratic)
    float getExponent() const;
    
    //! set the edge blending area for the left, top, right and bottom edges (values between 0 and 1)
    void setEdges(float left, float top, float right, float bottom);
    //! set the edge blending area for the left, top, right and bottom edges (values between 0 and 1)
    void setEdges(const glm::vec4 & edges);
    //! return the edge blending area for the left, top, right and bottom edges (values between 0 and 1)
    glm::vec4 getEdges() const;
    
    //! reset control points to undistorted image
    virtual void reset(const glm::vec2 & scale = glm::vec2(1.0f), const glm::vec2 & offset = glm::vec2(0.0f)) = 0;
    
    //! return the coordinates of the specified control point
    virtual glm::vec2 getControlPoint(size_t index);
    //! set the coordinates of the specified control point
    virtual void setControlPoint(size_t index, const glm::vec2 & pos);
    //! move the specified control point
    virtual void moveControlPoint(size_t index, const glm::vec2 & shift);
    //! get the number of control points
    virtual size_t getNumControlPoints() const;
    //! get the index of the currently selected control point
    virtual std::vector<size_t> getSelectedControlPoints() const;
    //! select one of the control points
    virtual void selectControlPoint(size_t index);
    //! deselect the selected control point
    virtual void deselectControlPoint(size_t index);
    //! deselect the selected control point
    virtual void deselectAllControlPoints();
    //! return the index of the closest control point, as well as the distance in pixels
    virtual size_t findClosestControlPoint(const glm::vec2 & pos, float * distance);
    
    //!return a list of controlpoints inside a specific area
    virtual std::vector<size_t> getControlPointsInArea(const ofRectangle& area);
    
    //! return the number of control points columns
    size_t getNumControlsX() const;
    //! return the number of control points rows
    size_t getNumControlsY() const;
    
    virtual void rotateClockwise() = 0;
    virtual void rotateCounterclockwise() = 0;
    
    virtual void flipHorizontal() = 0;
    virtual void flipVertical() = 0;
    
    virtual bool handleCursorDown(const glm::vec2 & pos);
    virtual bool handleCursorDrag(const glm::vec2 & pos);
    
    virtual bool handleWindowResize(int width, int height);
    
    virtual void loadPreset(const std::string& preset);
        
protected:
    
    //! draw a warped texture
    void draw(const ofTexture & texture);
    //! draw a specific area of a warped texture
    void draw(const ofTexture & texture, const ofRectangle & srcBounds);
    //! draw a specific area of a warped texture to a specific region
    void draw(const ofTexture & texture, const ofRectangle & srcBounds, const ofRectangle & dstBounds);
    
    //! adjust both the source and destination rectangles so that they are clipped against the warp's content
    bool clip(ofRectangle & srcBounds, ofRectangle & dstBounds) const;
    
    //! draw a specific area of a warped texture to a specific region
    virtual void drawTexture(const ofTexture & texture, const ofRectangle & srcBounds, const ofRectangle & dstBounds) = 0;
    //! draw the warp's controls interface
    virtual void drawControls() = 0;
    
    //! draw a control point in the preset color
    void queueControlPoint(const glm::vec2 & pos, bool selected = false, bool attached = false);
    //! draw a control point in the specified color
    void queueControlPoint(const glm::vec2 & pos, const ofFloatColor & color, float scale = 1.0f);
    
    //! setup the control points instanced vbo
    void setupControlPoints();
    //! draw the control points
    void drawControlPoints();
    
protected:
    
    static std::string sSaveFilename;
    
    virtual void handleRemoteUpdate(RemoteUIServerCallBackArg & arg);
    
    bool saveGroup{false};
    bool remoteEditMode{false};
    
    virtual void addControlPoints();
    virtual void removeControlPoints();
    
    void saveControlPoints(const std::filesystem::path& file);
    void loadControlPoints(const std::filesystem::path& file);
    
    void drawControlPointNames();

    std::string ctrlptPrefix;
    std::string warpName;
    std::string remoteGroupName;
    std::string currentPreset;
    std::filesystem::path saveLocation;
    ofRectangle srcArea;
    ofRectangle drawArea;
    bool show;
    
    //from ofxWarpbase
    
    WarpSettings::Type type;
    
    bool editing;
    bool dirty;
    
    float width;
    float height;
    
    glm::vec2 windowSize;
    
    float brightness;
    
    int numControlsX;
    int numControlsY;
    std::vector<glm::vec2> controlPoints;
    
    struct Selection {
        size_t index;
        glm::vec2 offset;
    };
    
    std::list<Selection> selectedIndices;
  
    glm::vec3 luminance;
    glm::vec3 gamma;
    float exponent;
    glm::vec4 edges;
    
    static const int MAX_NUM_CONTROL_POINTS = 1024;
        
    typedef enum
    {
        INSTANCE_POS_SCALE_ATTRIBUTE = 5,
        INSTANCE_COLOR_ATTRIBUTE = 6
    } Attribute;
    
    typedef struct ControlData
    {
        glm::vec2 pos;
        float scale;
        float dummy;
        ofFloatColor color;
        
        ControlData()
        {}
        
        ControlData(const glm::vec2 & pos, const ofFloatColor & color, float scale)
        : pos(pos)
        , color(color)
        , scale(scale)
        {}
    } ControlData;
    
    std::vector<ControlData> controlData;
    ofVboMesh controlMesh;
    ofShader controlShader;
    
};
