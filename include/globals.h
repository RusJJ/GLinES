#ifndef _GLIN_GLOBALS_H
#define _GLIN_GLOBALS_H

#include "GLES.h"

#ifdef USE_MAP_FOR_SHADERS_DESC
    #include <unordered_map>
#endif // USE_MAP_FOR_SHADERS_DESC

struct vector3_t
{
    float x, y, z;
};

struct vector4_t
{
    float x, y, z, w;
};

struct render_list_t
{
    bool begin = false;
    vector4_t color = {1.0f, 1.0f, 1.0f, 1.0f};
};

struct shader_desc_t
{
    GLuint shader = 0;
    bool vertexShader = false;
};

struct texture_desc_t
{
    GLuint id = 0;
    unsigned int width;
    unsigned int height;
};

struct program_arb_t
{
    GLuint id = 0;
    GLuint shader = 0;
    GLenum type = 0;
    char* src = NULL;
    bool vertexShader = false;
};

struct glstate_t
{
    bool enabledVertProgARB = false;
    bool enabledFragProgARB = false;
    texture_desc_t* activeTexture = nullptr;
};

struct arbstate_t
{
    char* errorStr = NULL;
    int errorPtr = -1;
    program_arb_t* activeVert = NULL;
    program_arb_t* activeFrag = NULL;
};

struct glin_globals_t
{
    render_list_t render;
    glstate_t gl;
    arbstate_t arb;

    GLenum lastPrimitiveMode;
    GLenum lastMatrixMode = GL_MODELVIEW;
    #ifdef USE_MAP_FOR_SHADERS_DESC
        std::unordered_map<GLuint, shader_desc_t*> shaders;
        std::unordered_map<GLuint, program_arb_t*> programsARB;
        std::unordered_map<GLuint, shader_desc_t*> textures;
    #else
        shader_desc_t* shaders[MAX_COUNT_OF_SAVED_SHADERS] = { 0 };
        program_arb_t* programsARB[MAX_COUNT_OF_SAVED_ARB_PROGS] = { 0 };
        texture_desc_t* textures[MAX_COUNT_OF_SAVED_TEXTURES] = { 0 };
    #endif // USE_MAP_FOR_SHADERS_DESC
};

extern glin_globals_t* globals;

#endif // _GLIN_GLOBALS_H