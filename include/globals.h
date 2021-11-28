#ifndef _GLIN_GLOBALS_H
#define _GLIN_GLOBALS_H

#include "GLES.h"

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

struct vector3_t
{
    float x, y, z;
};

struct vector4_t
{
    float x, y, z, w;
};

// globals.render
struct render_list_t
{
    bool begin = false;
    vector4_t color = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat mvp_matrix[16] = { 0.0f };
    GLfloat mv_matrix_inv[16] = { 0.0f };
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
    float clipPlanes[6][4] = { 0.0f };
    texture_desc_t* activeTexture = nullptr;
    GLuint activeQuery = 0;
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
    bool hasAlphaFuncQCOM;
    bool hasTextureLods;
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

    GLenum lastPrimitiveMode;
    GLenum lastMatrixMode = GL_MODELVIEW;
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