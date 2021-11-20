#include "GLES.h"
#include <dlfcn.h>

#include "wrapped.h"
#include "gl_buffer.h"
#include "gl_matrix.h"
#include "gl_object.h"
#include "gl_render.h"
#include "gl_shader_conv.h"
#include "gl_texture.h"

//////////////////////////////////////////////////////////////////////////////////////////////

#if defined(SYS64)
    #define RET_CMP(__fn_name, __fn) if(strcmp(name, __fn_name) == 0) { DBG("GLinES returned 0x%LX for %s", (unsigned long long)__fn, __fn_name); return (void*)__fn; }
#else
    #define RET_CMP(__fn_name, __fn) if(strcmp(name, __fn_name) == 0) { DBG("GLinES returned 0x%X for %s", (unsigned int)__fn, __fn_name); return (void*)__fn; }
#endif
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

//////////////////////////////////////////////////////////////////////////////////////////////

const char* pszGLExtensions = 
    #include "GL_Exts.inl"
;

//////////////////////////////////////////////////////////////////////////////////////////////

void *(*pSetGetProcAddr)(const char* name) = NULL;
void* GLIN_Stub0(void* param, ...) // Returns 0
{
    return NULL;
}

const GLubyte* WRAP(glGetString(GLenum name))
{
    switch(name)
    {
        case GL_VERSION:
            return (GLubyte*)"4.4";

        case GL_VENDOR:
            return (GLubyte*)"RusJJ aka [-=KILL MAN=-]";
            
        case GL_RENDERER:
            return (GLubyte*)"GLinES";
            
        case GL_SHADING_LANGUAGE_VERSION:
            return (GLubyte*)"2.1 over GLinES";
            
        case GL_EXTENSIONS:
            return (GLubyte*)pszGLExtensions;

        case 0x8874: //GL_PROGRAM_ERROR_STRING_ARB:
            return (GLubyte*)globals->arb.errorStr;
    }
    if((name & 0x10000) != 0) return glGetString(name - 0x10000);
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
// Main
    GLIN_MAP(glGetString);
    GLIN_ALL(glCompileShader);
    GLIN_ALL(glMapBuffer);
    GLIN_ALL(glDrawBuffer);
    GLIN_MAP(glColor4f);
    GLIN_MAP(glColor4sv);
    GLIN_ALL(glColorMaskIndexed);
// Object
    GLIN_ALL(glGetInfoLog);
    GLIN_ALL(glDeleteObject);
    GLIN_ALL(glGetObjectParameterfv);
    GLIN_ALL(glGetObjectParameteriv);
// Others
    GLIN_ALL(glGenPrograms); // GL4ES
    GLIN_ALL(glDeletePrograms);
// Probably complete
    GLIN_ALL(glBindProgram); // GL4ES
    GLIN_ALL(glProgramString);
    GLIN_ALL(glGetProgramString);
// Incomplete
    GLIN_ALL(glGetCompressedTexImage);
    GLIN_ALL(glGetTexImage);
    GLIN_MAP(glBegin);
    GLIN_MAP(glEnd);
    GLIN_MAP(glOrtho);
    GLIN_ALL(glMatrixMode);
    GLIN_MAP(glVertex3f); // GL4ES
// Not implemented
    GLIN_ALL(glClipPlane);
    GLIN_ALL(glTexCoord2f);
    GLIN_ALL(glPolygonMode);
    GLIN_ALL(glPopAttrib);
    GLIN_ALL(glPushAttrib);
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
// Default ES 3.x functions
    #include "ES3_Funcs.inl"
// Unknown function?
    MSG("GLIN_GetProcAddress(\"%s\") returned NULL!", name);
    return NULL;
}

// If we're a replacement for GL4ES:
GLINAPI void* EXPORT gl4es_GetProcAddress(const char* name) { return GLIN_GetProcAddress(name); }
GLINAPI void EXPORT initialize_gl4es() { DBG("GLinES doesn`t need to be initialized!"); }
GLINAPI void EXPORT set_getprocaddress(void *(*new_proc_address)(const char *)) { GLIN_SetProcAddr(new_proc_address); }

// Our own things
static glin_globals_t globalsLocal;
glin_globals_t* globals = &globalsLocal;