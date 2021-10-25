#ifndef _GLIN_GLOBALS_H
#define _GLIN_GLOBALS_H

#include "GLES.h"

#ifdef USE_MAP_FOR_SHADERS_DESC
    #include <unordered_map>
#endif // USE_MAP_FOR_SHADERS_DESC

struct render_list_t
{

};

struct shader_desc_t
{
    GLuint shader = 0;
    bool isVertexShader = false;
};

struct glin_globals_t
{
    GLenum lastMatrixMode = GL_MODELVIEW;
    #ifdef USE_MAP_FOR_SHADERS_DESC
        std::unordered_map<GLuint, shader_desc_t*> shaders;
    #else
        shader_desc_t* shaders[16384] = { 0 };
    #endif // USE_MAP_FOR_SHADERS_DESC
};

extern glin_globals_t* globals;

#endif // _GLIN_GLOBALS_H