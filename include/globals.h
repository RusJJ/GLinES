#ifndef _GLIN_GLOBALS_H
#define _GLIN_GLOBALS_H

#include "GLES.h"
#include <vector>

#ifdef USE_MAP_FOR_SHADERS_DESC
    #include <unordered_map>
#endif // USE_MAP_FOR_SHADERS_DESC

#include <time.h>
inline unsigned long long GetClock()
{
	static timespec out;
	clock_gettime(CLOCK_MONOTONIC_RAW, &out);
	return ((unsigned long long)out.tv_sec)*1000000000LL + out.tv_nsec;
}

struct vector2_t
{
    bool operator==(const vector2_t& v)
    {
        return (x == v.x && y == v.y);
    }
    float x, y;
};
struct vector3_t
{
    bool operator==(const vector3_t& v)
    {
        return (x == v.x && y == v.y && z == v.z);
    }
    float x, y, z;
};
struct vector4_t
{
    bool operator==(const vector4_t& v)
    {
        return (x == v.x && y == v.y && z == v.z && w == v.w);
    }
    float x, y, z, w;
};
struct matrix4_t
{
    float m[16];
    
    inline GLfloat* data()
    {
        return (GLfloat*)&m[0];
    }
    bool operator==(const matrix4_t& v)
    {
        for(int i = 0; i < 16; ++i) if(m[i] != v.m[i]) return false;
        return true;
    }
    matrix4_t operator*(const matrix4_t& v)
    {
        return MulRet(v);
    }
    matrix4_t operator*(const float* v)
    {
        return MulRet(*(const matrix4_t*)v);
    }
    matrix4_t& operator*=(const matrix4_t& v)
    {
        *this = *this * v;
        return *this;
    }
    matrix4_t& operator*=(const float* v)
    {
        *this = *this * *(const matrix4_t*)v;
        return *this;
    }
    matrix4_t MulRet(const matrix4_t& v)
    {
        matrix4_t res;
        for (int col = 0; col < 4; ++col)
        {
            for (int row = 0; row < 4; ++row)
            {
                res.m[col * 4 + row] = 
                    m[0 * 4 + row] * v.m[col * 4 + 0] +
                    m[1 * 4 + row] * v.m[col * 4 + 1] +
                    m[2 * 4 + row] * v.m[col * 4 + 2] +
                    m[3 * 4 + row] * v.m[col * 4 + 3];
            }
        }
        return res;
    }
    matrix4_t TransposeRet()
    {
        
    }
    
    static inline matrix4_t Identity()
    {
        return matrix4_t
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
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
        for(int i = 0; i < MAX_COUNT_OF_MATRIX_STACK; ++i)
        {
            matrices[i] = matrix4_t::Identity();
        }
    }
    inline matrix4_t& Push()
    {
        if(pos < MAX_COUNT_OF_MATRIX_STACK-1) ++pos;
        return matrices[pos];
    }
    inline matrix4_t& Current()
    {
        return matrices[pos];
    }
    inline void Pop()
    {
        if(pos > 0) --pos;
    }
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
    GLint colorSize = 4; GLenum colorType = GL_FLOAT; GLsizei colorStride = 0; GLuint colorBuffer = 0;
    GLenum normalType = GL_FLOAT; GLsizei normalStride = 0; GLuint normalBuffer = 0;
    texcoord_state_t texCoord[8];
};

// globals.ff
struct fixed_light_t
{
    
};
struct fixed_func_state_t
{
    bool lightingEnabled = false;
    bool normalizeEnabled = false;
    bool fogEnabled = false;
    bool logicOpEnabled = false;
    GLenum logicOpMode = GL_COPY;

    bool lightEnabled[8] = { false };
    float lightPos[8][4] = { {0,0,1,0} };
    float lightAmbient[8][4] = { {0,0,0,1} };
    float lightSpecular[8][4] = { {0,0,0,1} };
    float lightDiffuse[8][4] = { {0,0,0,1} };
    float lightSpotDir[8][4] = { {0,0,0,1} };
    float lightSpotExponent[8] = { 0 };
    float lightSpotCutoff[8] = { 0 };
    float lightAttenuation[8][3] = { {0,0,0} };
    
    float matAmbient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
    float matDiffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
    float matSpecular[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float matEmission[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float matShininess = 0.0f;

    GLenum fogMode = GL_EXP;
    vector4_t fogColor = {0,0,0,0};
    float fogDensity = 1.0f;
    float fogStart = 0.0f;
    float fogEnd = 1.0f;
    
    float clipPlanes[6][4] = { { 0.0f } } ;
    bool clipPlaneOn[6] = { false };

    GLenum shadeModel = GL_SMOOTH;
    GLint texEnvMode = GL_MODULATE; // GL_MODULATE, GL_DECAL, GL_BLEND, GL_REPLACE
    GLint activeTextureUnit = 0;
};

// globals.render
struct render_list_t
{
    bool begin = false;
    bool texture = false;
    vector4_t color = {1.0f, 1.0f, 1.0f, 1.0f};
    vector2_t texcoord = {0.0f, 0.0f};
    vector3_t normal = {0.0f, 0.0f, 1.0f};
    GLfloat mvp_matrix[16] = { 0.0f };
    
    std::vector<vector3_t> vertices;
    std::vector<vector4_t> colors;
    std::vector<vector2_t> texcoords;
    std::vector<vector3_t> normals;
    
    GLuint fixedVAO = 0;
    GLuint fixedVBO[11] = { 0 };
    
    vector4_t ambient = {0.2f, 0.2f, 0.2f, 1.0f};
};

// globals.shaders[*]
struct shader_desc_t
{
    GLuint shader = 0;
    bool vertexShader = false;
};

// globals.textures[*]
struct texture_desc_t
{
    GLuint id = 0;
    GLenum target = 0;
    unsigned int width;
    unsigned int height;
};

// globals.queries[*]
struct query_desc_t
{
    GLuint id;
    GLenum target;
    bool active;
    GLuint start;
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
    bool enabledVertProgARB = false;
    bool enabledFragProgARB = false;
    char lastPolygonMode = 0; // GL_FILL
    texture_desc_t* activeTexture = nullptr;
    GLuint activeQuery = 0;
    GLuint activeDrawBuffer = GL_COLOR_ATTACHMENT0;
    GLuint activeReadBuffer = GL_COLOR_ATTACHMENT0;
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
    
    bool hasAlphaFuncQCOM;
    bool hasTextureLods;
    bool hasDXT;
    bool hasClipCull; // GL_EXT_clip_cull_distance
};

// globals
struct glin_globals_t
{
    glin_globals_t()
    {
        MSG("Initializing GLinES...");
    }

    extensions_t ext;
    render_list_t render;
    glstate_t gl;
    arbstate_t arb;
    matrices_type_stack_t matrix;
    fixed_func_state_t ff;
    client_state_t client;

    GLenum lastPrimitiveMode;
    #ifdef USE_MAP_FOR_SHADERS_DESC
        std::unordered_map<GLuint, shader_desc_t*> shaders;
        std::unordered_map<GLuint, program_arb_t*> programsARB;
        std::unordered_map<GLuint, shader_desc_t*> textures;
        std::unordered_map<GLuint, query_desc_t*> queries;
    #else
        shader_desc_t* shaders[MAX_COUNT_OF_SAVED_SHADERS] = { 0 };
        program_arb_t* programsARB[MAX_COUNT_OF_SAVED_ARB_PROGS] = { 0 };
        texture_desc_t* textures[MAX_COUNT_OF_SAVED_TEXTURES] = { 0 };
        query_desc_t* queries[MAX_COUNT_OF_SAVED_QUERIES] = { 0 };
    #endif // USE_MAP_FOR_SHADERS_DESC
};

extern glin_globals_t* globals;

#endif // _GLIN_GLOBALS_H
