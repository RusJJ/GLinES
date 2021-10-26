#include "GLES.h"
#include <dlfcn.h>

#include "wrapped.h"
#include "gl_buffer.h"
#include "gl_matrix.h"
#include "gl_object.h"
#include "gl_render.h"
#include "gl_shader_conv.h"

#define RET_CMP(__fn_name, __fn) if(strcmp(name, __fn_name) == 0) { DBG("GLinES returned 0x%X for %s", (unsigned int)__fn, __fn_name); return (void*)__fn; }

#define GL_MAP(__fn_name)                       RET_CMP(#__fn_name,       __fn_name)
#define GL_ARB(__fn_name)                       RET_CMP(#__fn_name "ARB", __fn_name)
#define GL_EXT(__fn_name)                       RET_CMP(#__fn_name "EXT", __fn_name)
#define GL_ALL(__fn_name)                       GL_MAP(__fn_name); GL_ARB(__fn_name); GL_EXT(__fn_name)
#define AS_MAP(__fn_name, __ret_fn_name)        RET_CMP(#__fn_name,       __ret_fn_name)
#define AS_ARB(__fn_name, __ret_fn_name)        RET_CMP(#__fn_name "ARB", __ret_fn_name)
#define AS_EXT(__fn_name, __ret_fn_name)        RET_CMP(#__fn_name "EXT", __ret_fn_name)
#define AS_ALL(__fn_name, __ret_fn_name)        AS_MAP(__fn_name, __ret_fn_name); AS_ARB(__fn_name, __ret_fn_name); AS_EXT(__fn_name, __ret_fn_name)
#define GLIN_MAP(__fn_name)                     RET_CMP(#__fn_name,       GLIN_Wrap_ ## __fn_name)
#define GLIN_ARB(__fn_name)                     RET_CMP(#__fn_name "ARB", GLIN_Wrap_ ## __fn_name)
#define GLIN_EXT(__fn_name)                     RET_CMP(#__fn_name "EXT", GLIN_Wrap_ ## __fn_name)
#define GLIN_ALL(__fn_name)                     GLIN_MAP(__fn_name); GLIN_ARB(__fn_name); GLIN_EXT(__fn_name)
#define AS_GLIN_MAP(__fn_name, __ret_fn_name)   RET_CMP(#__fn_name,       GLIN_Wrap_ ## __ret_fn_name)
#define AS_GLIN_ARB(__fn_name, __ret_fn_name)   RET_CMP(#__fn_name "ARB", GLIN_Wrap_ ## __ret_fn_name)
#define AS_GLIN_EXT(__fn_name, __ret_fn_name)   RET_CMP(#__fn_name "EXT", GLIN_Wrap_ ## __ret_fn_name)
#define AS_GLIN_ALL(__fn_name, __ret_fn_name)   AS_GLIN_MAP(__fn_name, __ret_fn_name); AS_GLIN_ARB(__fn_name, __ret_fn_name); AS_GLIN_EXT(__fn_name, __ret_fn_name)
#define STUB(__fn_name)                         RET_CMP(#__fn_name,       GLIN_Stub0)
#define STUB_ARB(__fn_name)                     RET_CMP(#__fn_name "ARB", GLIN_Stub0)
#define STUB_EXT(__fn_name)                     RET_CMP(#__fn_name "EXT", GLIN_Stub0)
#define STUB_ALL(__fn_name)                     STUB(__fn_name); STUB_ARB(__fn_name); STUB_EXT(__fn_name)

const char* pszGLExtensions = "GL_EXT_abgr "
                "GL_EXT_packed_pixels "
                "GL_EXT_compiled_vertex_array "
                "GL_EXT_compiled_vertex_arrays "
                "GL_ARB_vertex_buffer_object "
                "GL_ARB_vertex_array_object "
                "GL_ARB_vertex_buffer "
                "GL_EXT_vertex_array "
                "GL_EXT_secondary_color "
                "GL_ARB_multitexture "
                "GL_ARB_texture_border_clamp "
                "GL_ARB_texture_env_add "
                "GL_EXT_texture_env_add "
                "GL_ARB_texture_env_combine "
                "GL_EXT_texture_env_combine "
                "GL_ARB_texture_env_crossbar "
                "GL_EXT_texture_env_crossbar "
                "GL_ARB_texture_env_dot3 "
                "GL_EXT_texture_env_dot3 "
                "GL_SGIS_generate_mipmap "
                "GL_EXT_draw_range_elements "
                "GL_EXT_bgra "
                "GL_ARB_texture_compression "
                "GL_EXT_texture_compression_s3tc "
                "GL_OES_texture_compression_S3TC "
                "GL_EXT_texture_compression_dxt1 "
                "GL_EXT_texture_compression_dxt3 "
                "GL_EXT_texture_compression_dxt5 "
                "GL_ARB_point_parameters "
                "GL_EXT_point_parameters "
                "GL_EXT_stencil_wrap "
                "GL_SGIS_texture_edge_clamp "
                "GL_EXT_texture_edge_clamp "
                "GL_EXT_direct_state_access "
                "GL_EXT_multi_draw_arrays "
                "GL_SUN_multi_draw_arrays "
                "GL_ARB_multisample "
                "GL_EXT_texture_object "
                "GL_EXT_polygon_offset "
                "GL_GL4ES_hint "
                "GL_ARB_draw_elements_base_vertex "
                "GL_EXT_draw_elements_base_vertex "
                "GL_ARB_map_buffer_range "
                "GL_NV_blend_square "
                "GL_ARB_texture_rectangle "
                "GL_ARB_vertex_array_bgra "
                "GL_ARB_texture_non_power_of_two "
                "GL_EXT_blend_color "
                "GL_EXT_blend_minmax "
                "GL_EXT_blend_equation_separate "
                "GL_EXT_blend_func_separate "
                "GL_EXT_blend_subtract "
                "GL_EXT_texture_filter_anisotropic "
                "GL_ARB_texture_mirrored_repeat "
                "GL_ARB_framebuffer_object "
                "GL_EXT_framebuffer_object "
                "GL_EXT_packed_depth_stencil "
                "GL_EXT_framebuffer_blit "
                "GL_ARB_draw_buffers "
                "GL_EXT_draw_buffers2 "
                "GL_ARB_point_sprite "
                "GL_ARB_texture_cube_map "
                "GL_EXT_texture_cube_map "
                "GL_EXT_texture_rg "
                "GL_ARB_texture_rg "
                "GL_EXT_texture_float "
                "GL_ARB_texture_float "
                "GL_EXT_color_buffer_float "
                "GL_EXT_color_buffer_half_float "
                "GL_EXT_depth_texture "
                "GL_ARB_depth_texture "
                "GL_EXT_fog_coord "
                "GL_EXT_separate_specular_color "
                "GL_EXT_rescale_normal "
                "GL_ARB_ES2_compatibility "
                "GL_ARB_ES3_compatibility "
                "GL_ARB_fragment_shader "
                "GL_ARB_vertex_shader "
                "GL_ARB_shader_objects "
                "GL_ARB_shading_language_100 "
                "GL_ATI_texture_env_combine3 "
                "GL_ATIX_texture_env_route "
                "GL_NV_texture_env_combine4 "
                "GL_NV_fog_distance "
                "GL_ARB_draw_instanced "
                "GL_ARB_instanced_arrays "
                "GL_ARB_vertex_program "
                "GL_ARB_fragment_program "
                "GL_EXT_program_parameters "
                "GL_EXT_texture_sRGB_decode "
                "GL_ARB_get_program_binary ";

void *(*pSetGetProcAddr)(const char* name) = NULL;
void* GLIN_Stub0(void* param, ...) // Returns 0
{
    //DBG("Stub0 is being called!");
    return NULL;
}
void* GLIN_Stub1(void* param, ...) // Returns 1
{
    //DBG("Stub1 is being called!");
    return (void*)1;
}

const GLubyte* GLIN_Wrap_glGetString(GLenum name)
{
    DBG("Someone is calling glGetString(0x%X)", name);
    switch(name)
    {
        case GL_VERSION:
            return (GLubyte*)"2.1.0";

        case GL_VENDOR:
            return (GLubyte*)"RusJJ aka [-=KILL MAN=-]";
            
        case GL_RENDERER:
            return (GLubyte*)"GLinES";
            
        case GL_SHADING_LANGUAGE_VERSION:
            return (GLubyte*)"2.1 using GLinES";
            
        case GL_EXTENSIONS:
            return (GLubyte*)pszGLExtensions;
    }
    if((name & 0x10000) != 0)
    {
        return glGetString(name - 0x10000);
    }
    return glGetString(name);
}

GLINAPI void EXPORT GLIN_SetProcAddr(void*(*fn)(const char*))
{
    pSetGetProcAddr = fn;
}

GLINAPI void* EXPORT GLIN_ProcAddr(void* lib, const char* name)
{
    if(pSetGetProcAddr != NULL) return pSetGetProcAddr(name);
    return dlsym(lib, name);
}

GLINAPI void* EXPORT GLIN_GetProcAddress(const char* name)
{
    GLIN_MAP(glGetString);
    GLIN_ALL(glCompileShader);
    GLIN_ALL(glMapBuffer);
    GLIN_ALL(glDrawBuffer);
    GLIN_MAP(glColor4f);
    GLIN_MAP(glColor4sv);
    // Object
    GLIN_ALL(glColorMaskIndexed);
    GLIN_ALL(glGetInfoLog);
    GLIN_ALL(glDeleteObject);
    GLIN_ALL(glGetObjectParameterfv);
    GLIN_ALL(glGetObjectParameteriv);

    GLIN_MAP(glBegin);
    GLIN_MAP(glEnd);
    GLIN_MAP(glOrtho);
    GLIN_ALL(glMatrixMode);
    GLIN_ALL(glBindProgram); // GL4ES
    GLIN_ALL(glGenPrograms); // GL4ES
    GLIN_MAP(glVertex3f); // GL4ES

    GLIN_ALL(glClipPlane);
    GLIN_ALL(glDeletePrograms);
    GLIN_ALL(glGetCompressedTexImage);
    GLIN_ALL(glGetTexImage);
    GLIN_ALL(glTexCoord2f);
    GLIN_ALL(glPolygonMode);
    GLIN_ALL(glPopAttrib);
    GLIN_ALL(glPushAttrib);
    GLIN_ALL(glProgramString);
    GLIN_ALL(glEnableClientState);
    GLIN_ALL(glDisableClientState);
    GLIN_ALL(glClientActiveTexture);
    GLIN_ALL(glVertexPointer);
    GLIN_ALL(glTexCoordPointer);
    GLIN_ALL(glProgramEnvParameters4fv);
    GLIN_ALL(glEnableIndexed);
    GLIN_ALL(glDisableIndexed);
    GLIN_ALL(glGetBooleanIndexedv);
    GLIN_ALL(glPushClientAttrib);
    GLIN_ALL(glPopClientAttrib);

    #include "ES3_Funcs.inl" // All OpenGLES 3.2 functions

    DBG("GLIN_GetProcAddress(\"%s\") returned NULL!", name);
    //return (void*)GLIN_Stub0;
    return NULL;
}

GLINAPI void EXPORT GLIN_Init()
{
    static bool bIsInited = false;
    if(bIsInited)
    {
        ERR("Someone is trying to initialize me one more time!");
        return;
    }
    bIsInited = true;

    DBG("GLinES is initialized!");
}

// Our own things
static struct glin_globals_t globalsLocal;
struct glin_globals_t* globals = &globalsLocal;

// If we're a replacement for GL4ES:
GLINAPI void* EXPORT gl4es_GetProcAddress(const char* name) { return GLIN_GetProcAddress(name); }
GLINAPI void EXPORT initialize_gl4es() { GLIN_Init(); }
GLINAPI void EXPORT set_getprocaddress(void *(*new_proc_address)(const char *)) { GLIN_SetProcAddr(new_proc_address); }