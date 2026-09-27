#ifndef _GLIN_GLOBALS_H
#define _GLIN_GLOBALS_H

#include "GLES.h"
#include <vector>
#include <functional>
#include <unordered_map>
#include <array>
#include <memory>
#include <type_traits>
#include <string>

#include <time.h>
inline unsigned long long GetClock()
{
    timespec out;
    clock_gettime(CLOCK_MONOTONIC_RAW, &out);
    return ((unsigned long long)out.tv_sec)*1000000000LL + out.tv_nsec;
}

static inline void f4tod(const float* f, GLdouble* d, int n = 4)
{
    for(int i = 0; i < n; ++i) d[i] = (GLdouble)f[i];
}
static inline float b2f(GLbyte v)   { return v < 0 ? v/128.0f  : v/127.0f; }
static inline float ub2f(GLubyte v) { return v/255.0f; }
static inline float s2f(GLshort v)  { return v < 0 ? v/32768.0f : v/32767.0f; }
static inline float i2f(GLint v)    { return v < 0 ? v/2147483648.0f : v/2147483647.0f; }
static inline float us2f(GLushort v){ return v / 65535.0f; }
static inline float ui2f(GLuint v)  { return v / 4294967295.0f; }

// Display lists macros
#define DLREC(fn) \
    if(globals->currentList != 0) { \
        auto _it = globals->lists.find(globals->currentList); \
        if(_it != globals->lists.end() && !_it->second->compiled) { \
            _it->second->commands.push_back([=](){ fn; }); \
        } \
        if(globals->currentListMode == GL_COMPILE) return; \
    } \
    list_record_guard_t _listGuard;

#define DLREC_DATA(ptr, count, fn) \
    if(globals->currentList != 0) { \
        using value_t = typename std::remove_cv<typename std::remove_pointer<decltype(ptr)>::type>::type; \
        std::vector<value_t> _values(ptr, ptr + (count)); \
        auto _it = globals->lists.find(globals->currentList); \
        if(_it != globals->lists.end() && !_it->second->compiled) _it->second->commands.push_back([=](){ fn; }); \
        if(globals->currentListMode == GL_COMPILE) return; \
    } \
    list_record_guard_t _listGuard;

#define DLREC_MAT3(ptr, fn) \
    if(globals->currentList != 0) { \
        std::array<GLfloat, 3*3> _m; \
        memcpy(_m.data(), ptr, 3*3*sizeof(GLfloat)); \
        auto _it = globals->lists.find(globals->currentList); \
        if(_it != globals->lists.end() && !_it->second->compiled) { \
            _it->second->commands.push_back([=](){ fn; }); \
        } \
        if(globals->currentListMode == GL_COMPILE) return; \
    } \
    list_record_guard_t _listGuard;

#define DLREC_MAT4(ptr, fn) \
    if(globals->currentList != 0) { \
        std::array<GLfloat, 4*4> _m; \
        memcpy(_m.data(), ptr, 4*4*sizeof(GLfloat)); \
        auto _it = globals->lists.find(globals->currentList); \
        if(_it != globals->lists.end() && !_it->second->compiled) { \
            _it->second->commands.push_back([=](){ fn; }); \
        } \
        if(globals->currentListMode == GL_COMPILE) return; \
    } \
    list_record_guard_t _listGuard;
    


struct vector2_t
{
    bool operator==(const vector2_t& v) const { return (x == v.x && y == v.y); }
    float x, y;
};
struct vector3_t
{
    bool operator==(const vector3_t& v) const { return (x == v.x && y == v.y && z == v.z); }
    float x, y, z;
};
struct vector4_t
{
    bool operator==(const vector4_t& v) const { return (x == v.x && y == v.y && z == v.z && w == v.w); }
    float x, y, z, w;
};
struct matrix3_t
{
    float m[9];
    inline GLfloat* data() { return (GLfloat*)&m[0]; }
    bool operator==(const matrix3_t& v) const
    {
        for(int i = 0; i < 9; ++i) if(m[i] != v.m[i]) return false;
        return true;
    }
};
struct matrix4_t
{
    float m[16];
    inline GLfloat* data() { return (GLfloat*)&m[0]; }
    bool operator==(const matrix4_t& v) const
    {
        for(int i = 0; i < 16; ++i) if(m[i] != v.m[i]) return false;
        return true;
    }
    matrix4_t operator*(const matrix4_t& v) const { return MulRet(v); }
    matrix4_t operator*(const float* v) const     { return MulRet(*(const matrix4_t*)v); }
    matrix4_t& operator*=(const matrix4_t& v) { *this = *this * v; return *this; }
    matrix4_t& operator*=(const float* v)     { *this = *this * *(const matrix4_t*)v; return *this; }
    matrix4_t MulRet(const matrix4_t& v) const
    {
        matrix4_t res;
        for (int col = 0; col < 4; ++col)
            for (int row = 0; row < 4; ++row)
                res.m[col * 4 + row] = 
                    m[0*4+row]*v.m[col*4+0] + m[1*4+row]*v.m[col*4+1] +
                    m[2*4+row]*v.m[col*4+2] + m[3*4+row]*v.m[col*4+3];
        return res;
    }
    matrix4_t TransposeRet() const
    {
        matrix4_t res;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                res.m[i*4+j] = m[j*4+i];
        return res;
    }
    static inline matrix4_t Identity()
    {
        return matrix4_t{ 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };
    }
};

// globals.matrix
struct matrix_stack_t
{
    unsigned char pos = 0;
    matrix4_t matrices[MAX_COUNT_OF_MATRIX_STACK];
    matrix_stack_t() { Reset(); }
    inline void Reset()
    {
        pos = 0;
        for(int i = 0; i < MAX_COUNT_OF_MATRIX_STACK; ++i) matrices[i] = matrix4_t::Identity();
    }
    inline matrix4_t& Push() { if(pos < MAX_COUNT_OF_MATRIX_STACK-1) ++pos; return matrices[pos]; }
    inline matrix4_t& Current() { return matrices[pos]; }
    inline void Pop() { if(pos > 0) --pos; }
};
struct matrices_type_stack_t
{
    GLenum mode = GL_MODELVIEW;
    matrix_stack_t projection;
    matrix_stack_t modelview;
    matrix_stack_t texture;
    inline matrix4_t& Push()
    {
        if(mode == GL_PROJECTION) return projection.Push();
        else if(mode == GL_TEXTURE) return texture.Push();
        else return modelview.Push();
    }
    inline matrix4_t& Current()
    {
        if(mode == GL_PROJECTION) return projection.Current();
        else if(mode == GL_TEXTURE) return texture.Current();
        else return modelview.Current();
    }
    inline void Pop()
    {
        if(mode == GL_PROJECTION) return projection.Pop();
        else if(mode == GL_TEXTURE) return texture.Pop();
        else return modelview.Pop();
    }
};

// globals.client
struct texcoord_state_t
{
    bool enabled = false;
    GLint texCoordSize = 4;
    GLenum texCoordType = GL_FLOAT;
    GLsizei texCoordStride = 0;
    GLuint texCoordBuffer = 0;
    const void* texCoordPtr = NULL;
    vector4_t texCoordColor = {0,0,0,0};
    unsigned char texCoordBlendLogic = 1;
};
struct client_state_t
{
    GLuint boundArrayBuffer = 0;
    GLuint boundElementBuffer = 0;
    GLuint boundPixelUnpackBuffer = 0;
    GLuint boundPixelPackBuffer = 0;
    GLuint boundUniformBuffer = 0;
    
    bool vertexArrayEnabled = false;
    bool colorArrayEnabled = false;
    bool normalArrayEnabled = false;

    const void* vertexPtr = NULL;
    const void* colorPtr = NULL;
    const void* normalPtr = NULL;

    GLint vertexSize = 4; GLenum vertexType = GL_FLOAT; GLsizei vertexStride = 0; GLuint vertexBuffer = 0;
    GLint colorSize = 4;  GLenum colorType = GL_FLOAT;  GLsizei colorStride = 0;  GLuint colorBuffer = 0;
    GLenum normalType = GL_FLOAT; GLsizei normalStride = 0; GLuint normalBuffer = 0;
    texcoord_state_t texCoord[8];
    GLint clientActiveTextureUnit = 0;

    GLint  secondaryColorSize   = 3;
    GLenum secondaryColorType   = GL_FLOAT;
    GLsizei secondaryColorStride = 0;
    GLuint secondaryColorBuffer = 0;
    const void* secondaryColorPtr = NULL;
};

// globals.ff
struct fixed_light_t
{
    vector4_t pos = {0,0,1,0};
    vector4_t ambient = {0,0,0,1};
    vector4_t diffuse = {0,0,0,1};
    vector4_t spec = {0,0,0,1};
    vector4_t dir = {0,0,0,1};
    float spotExp = 0.0f;
    float spotCutoff = 180.0f;
    float spotPad[2]; // std140
    float attenuationConst = 1.0f;
    float attenuationLinear = 0.0f;
    float attenuationQuad = 0.0f;
    float attenuationPad; // std140
};
struct fixed_func_state_t
{
    bool textureEnabled[8] = {};
    bool lightingEnabled = false;
    bool normalizeEnabled = false;
    bool fogEnabled = false;
    bool logicOpEnabled = false;
    GLenum logicOpMode = GL_COPY;

    bool lightEnabled[8] = { false };
    fixed_light_t lights[8];
    
    float matAmbient[4]   = {0.2f, 0.2f, 0.2f, 1.0f};
    float matDiffuse[4]   = {0.8f, 0.8f, 0.8f, 1.0f};
    float matSpecular[4]  = {0.0f, 0.0f, 0.0f, 1.0f};
    float matEmission[4]  = {0.0f, 0.0f, 0.0f, 1.0f};
    float matShininess = 0.0f;
    
    GLenum colorMaterialFace = GL_FRONT_AND_BACK;
    GLenum colorMaterialMode = GL_AMBIENT_AND_DIFFUSE;

    GLenum fogMode = GL_EXP;
    vector4_t fogColor = {0,0,0,0};
    float fogDensity = 1.0f;
    float fogStart = 0.0f;
    float fogEnd = 1.0f;
    
    float clipPlanes[6][4] = { { 0.0f } };
    bool clipPlaneOn[6] = { false };
    
    bool lightModelTwoSide = false;
    bool lightModelLocalViewer = false;
    bool affineTexcoord = false;

    GLenum shadeModel = GL_SMOOTH;
    GLint texEnvMode = GL_MODULATE;
    GLint activeTextureUnit = 0;

    bool alphaTestEnabled = false;
    GLenum alphaTestFunc = GL_ALWAYS;
    float alphaTestRef = 0.0f;

    float pointSize = 1.0f;
    float lineWidth = 1.0f;

    float pointAttenuation[3] = {1.0f, 0.0f, 0.0f};
    float pointFadeThreshold  = 1.0f;

    struct texgen_t
    {
        GLenum mode         = 0x2400; // GL_EYE_LINEAR
        float  objectPlane[4] = {0,0,0,0};
        float  eyePlane[4]    = {0,0,0,0};
        bool   enabled       = false;
    } texGen[4]; // S T R Q

    GLubyte  polygonStipple[128] = {0};
    bool     polygonStippleEnabled = false;
    GLint    lineStippleFactor  = 1;
    GLushort lineStipplePattern = 0xFFFF;
    bool     lineStippleEnabled = false;

    float pixelZoomX = 1.0f;
    float pixelZoomY = 1.0f;
};

// globals.render
struct render_list_t
{
    vector4_t rasterPos = {0.0f, 0.0f, 0.0f, 1.0f};
    bool rasterPosValid = false;
    
    bool begin = false;
    bool texture = false;
    bool colorMaterial = false;
    vector4_t color = {1.0f, 1.0f, 1.0f, 1.0f};
    vector2_t texcoord = {0.0f, 0.0f};
    vector3_t normal = {0.0f, 0.0f, 1.0f};
    GLfloat mvp_matrix[16] = { 0.0f };
    
    std::vector<vector4_t> vertices;
    std::vector<vector4_t> colors;
    std::vector<vector2_t> texcoords;
    std::vector<vector3_t> normals;
    
    GLuint fixedVAO = 0;
    GLuint fixedVBO[11] = { 0 };
    
    vector4_t ambient = {0.2f, 0.2f, 0.2f, 1.0f};
    
    GLenum lastPrimitiveMode = GL_TRIANGLES;
};

// globals.shaders[*]
struct shader_desc_t
{
    GLuint shader = 0;
    bool vertexShader = false;
    std::string source;
};

// globals.textures[*]
struct texture_desc_t
{
    GLuint id = 0;
    GLenum target = 0;
    GLenum baseFormat = GL_RGBA;
    unsigned int width = 0;
    unsigned int height = 0;
};

// globals.queries[*]
struct query_desc_t
{
    GLuint id;
    GLenum target;
    unsigned long long start;
    bool active;
};

// globals.programs[*]
struct program_t
{
    GLuint id = 0;
    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
    GLuint geometryShader = 0;
    GLuint attributes[4] = { 0xFFFFFFFF };
};

// globals.programsARB[*]
struct program_arb_t
{
    GLuint id = 0;
    GLuint shader = 0;
    GLenum type = 0;
    char* src = NULL;
    bool vertexShader = false;
};

// globals.gl
struct glstate_t
{
    bool primitiveRestart = false;
    GLuint restartIndex = 0;
    bool enabledVertProgARB = false;
    bool enabledFragProgARB = false;
    char lastPolygonMode = 0; // GL_FILL
    texture_desc_t* activeTexture = nullptr;
    GLuint activeQuery = 0;
    GLuint activeDrawBuffer = 0;
    GLuint activeReadBuffer = 0;
    GLenum activeTexUnit = GL_TEXTURE0;
    GLuint activeProgram = 0;
    unsigned long long queriesTimeOffset = GetClock();
};

// globals.arb
struct arbstate_t
{
    char* errorStr = NULL;
    int errorPtr = -1;
    program_arb_t* activeVert = NULL;
    program_arb_t* activeFrag = NULL;
};

// globals.ext
struct extensions_t
{
    bool checked_exts_for_shaders = false;
    bool checked_exts_for_textures = false;
    
    bool hasAlphaFuncQCOM = false;
    bool hasTextureLods = false;
    bool hasDXT = false;
    bool hasClipCull = false; // GL_EXT_clip_cull_distance
};

// globals.settings
struct settings_t
{
    bool matrix_transpose_invector = true;
};

// globals.list
struct display_list_t
{
    GLuint id = 0;
    bool compiled = false;
    std::vector<std::function<void()>> commands;
};

#define GL_CURRENT_BIT         0x00000001
#define GL_POINT_BIT           0x00000002
#define GL_LINE_BIT            0x00000004
#define GL_POLYGON_BIT         0x00000008
#define GL_LIGHTING_BIT        0x00000040
#define GL_FOG_BIT             0x00000080
#define GL_TEXTURE_BIT         0x00000400
#define GL_ENABLE_BIT          0x00002000
// globals.attribStack
struct attrib_snapshot_t
{
    bool primitiveRestart = false;
    fixed_func_state_t fixed;
    std::unordered_map<GLenum, GLboolean> nativeEnable;
    GLint blend[6] = {}, depthFunc = GL_LESS, cullFace = GL_BACK, frontFace = GL_CCW;
    GLint viewport[4] = {}, scissor[4] = {};
    GLfloat clearColor[4] = {}, blendColor[4] = {}, depthRange[2] = {}, clearDepth = 1;
    GLboolean colorMask[4] = {}, depthMask = GL_TRUE;

    GLbitfield mask = 0;

    // GL_CURRENT_BIT
    vector4_t color;
    vector2_t texcoord;
    vector3_t normal;

    // GL_ENABLE_BIT etc.
    bool lightingEnabled;
    bool normalizeEnabled;
    bool fogEnabled;
    bool logicOpEnabled;
    bool alphaTestEnabled;
    bool colorMaterial;
    bool texture;
    bool lightEnabled[8];
    bool clipPlaneOn[6];

    // GL_FOG_BIT
    GLenum fogMode;
    vector4_t fogColor;
    float fogDensity, fogStart, fogEnd;

    // GL_LIGHTING_BIT
    float matAmbient[4], matDiffuse[4], matSpecular[4], matEmission[4];
    float matShininess;
    GLenum colorMaterialFace, colorMaterialMode;
    bool lightModelTwoSide, lightModelLocalViewer;
    GLenum shadeModel;
    fixed_light_t lights[8];
    vector4_t ambient;

    // GL_TEXTURE_BIT
    GLint texEnvMode;

    // GL_POINT_BIT / GL_LINE_BIT
    float pointSize;
    float lineWidth;

    // GL_POLYGON_BIT
    char lastPolygonMode;

    // GL_COLOR_BUFFER_BIT (alpha test)
    GLenum alphaTestFunc;
    float alphaTestRef;

    // GL_TEXTURE_BIT
    fixed_func_state_t::texgen_t texGen[4];
    bool texGenEnabled[4]; // S T R Q

    // GL_POLYGON_BIT
    bool polygonStippleEnabled;
    GLubyte polygonStipple[128];

    // GL_LINE_BIT
    bool lineStippleEnabled;
    GLint lineStippleFactor;
    GLushort lineStipplePattern;
};

#define GL_CLIENT_PIXEL_STORE_BIT   0x00000001
#define GL_CLIENT_VERTEX_ARRAY_BIT  0x00000002
// globals.clientAttribStack
struct client_attrib_snapshot_t
{
    GLint pixelStore[10] = {};
    client_state_t client;

    GLbitfield mask = 0;

    // GL_CLIENT_VERTEX_ARRAY_BIT
    bool vertexArrayEnabled;
    bool colorArrayEnabled;
    bool normalArrayEnabled;
    GLint clientActiveTextureUnit;
    texcoord_state_t texCoord[8];
    GLint vertexSize; GLenum vertexType; GLsizei vertexStride; GLuint vertexBuffer; const void* vertexPtr;
    GLint colorSize;  GLenum colorType;  GLsizei colorStride;  GLuint colorBuffer;  const void* colorPtr;
    GLenum normalType; GLsizei normalStride; GLuint normalBuffer; const void* normalPtr;

    // GL_CLIENT_PIXEL_STORE_BIT
    GLuint boundPixelUnpackBuffer;
    GLuint boundPixelPackBuffer;
};

#define GL_COMPILE                  0x1300
#define GL_S                        0x2000
#define GL_TEXTURE_GEN_MODE         0x2500
#define GL_OBJECT_PLANE             0x2501
#define GL_EYE_PLANE                0x2502
struct fixed_program_t;
struct shared_objects_t
{
    std::unordered_map<GLuint, display_list_t*> lists;
    std::unordered_map<GLuint, shader_desc_t*> shaders;
    std::unordered_map<GLuint, program_arb_t*> programsARB;
    std::unordered_map<GLuint, texture_desc_t*> textures;
    std::unordered_map<GLuint, query_desc_t*> queries;
    ~shared_objects_t()
    {
        for(auto& item : lists) delete item.second;
        for(auto& item : shaders) delete item.second;
        for(auto& item : programsARB) { if(item.second) delete[] item.second->src; delete item.second; }
        for(auto& item : textures) delete item.second;
        for(auto& item : queries) delete item.second;
    }
};

// globals
struct glin_globals_t
{
    glin_globals_t(std::shared_ptr<shared_objects_t> shared = std::make_shared<shared_objects_t>())
        : objects(shared), lists(shared->lists), shaders(shared->shaders), programsARB(shared->programsARB), textures(shared->textures), queries(shared->queries)
    {
        MSG("Initializing GLinES...");
        ff.lights[0].ambient = {0,0,0,1};
        ff.lights[0].spec    = {1,1,1,1};
        ff.lights[0].diffuse = {1,1,1,1};
    }
    
    GLuint listBase = 0;
    GLuint currentList = 0;
    unsigned int listDepth = 0;
    GLenum error = GL_NO_ERROR;
    GLenum currentListMode = 0;

    extensions_t ext;
    render_list_t render;
    glstate_t gl;
    arbstate_t arb;
    matrices_type_stack_t matrix;
    fixed_func_state_t ff;
    client_state_t client;
    settings_t settings;
    std::shared_ptr<shared_objects_t> objects;
    std::unordered_map<GLuint, display_list_t*>& lists;
    std::unordered_map<GLuint, shader_desc_t*>& shaders;
    std::unordered_map<GLuint, program_arb_t*>& programsARB;
    std::unordered_map<GLuint, texture_desc_t*>& textures;
    std::unordered_map<GLuint, query_desc_t*>& queries;
    std::unordered_map<unsigned long long, std::shared_ptr<fixed_program_t>> fixedPrograms;
    std::vector<attrib_snapshot_t> attribStack;
    std::vector<client_attrib_snapshot_t> clientAttribStack;
    ~glin_globals_t() { delete[] arb.errorStr; }

};

extern thread_local glin_globals_t* globals;
inline void SetError(GLenum error) { if(globals->error == GL_NO_ERROR) globals->error = error; }

struct list_record_guard_t
{
    GLuint list;
    list_record_guard_t() : list(globals->currentList) { globals->currentList = 0; }
    ~list_record_guard_t() { globals->currentList = list; }
};

#endif // _GLIN_GLOBALS_H
