#include "GLES.h"
#include <dlfcn.h>

#include "wrapped.h"
#include "gl_buffer.h"
#include "gl_matrix.h"
#include "gl_object.h"
#include "gl_queries.h"
#include "gl_render.h"
#include "gl_shader.h"
#include "gl_texture.h"

//////////////////////////////////////////////////////////////////////////////////////////////

#if defined(SYS64)
    #define RET_CMP(__fn_name, __fn) if(strcmp(name, __fn_name) == 0) { DBG("GLinES returned 0x%016llX for %s", (unsigned long long)__fn, __fn_name); return (void*)__fn; }
#else
    #define RET_CMP(__fn_name, __fn) if(strcmp(name, __fn_name) == 0) { DBG("GLinES returned 0x%08X for %s", (unsigned int)__fn, __fn_name); return (void*)__fn; }
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

typedef void *(*getprocaddressType)(const char *);
getprocaddressType pGetProcAddr = NULL;
void* GLIN_Stub0(void* param, ...) // Returns 0
{
    return NULL;
}

const GLubyte* WRAP(glGetString(GLenum name))
{
    switch(name)
    {
        case GL_VERSION:
            return (GLubyte*)"3.3"; // Lets get it working one by one.

        case GL_VENDOR:
            return (GLubyte*)"RusJJ aka [-=KILL MAN=-]";
            
        case GL_RENDERER:
            return (GLubyte*)"GLinES";
            
        case GL_SHADING_LANGUAGE_VERSION:
            return (GLubyte*)"3.30 via GLinES " GLINES_VERSION_STR;
            
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
    pGetProcAddr = fn;
}

GLINAPI void* EXPORT GLIN_ProcAddr(void* lib, const char* name)
{
    void* ret = NULL;
    if(pGetProcAddr != NULL) ret = pGetProcAddr(name);
    if(ret == NULL) ret = dlsym(lib, name);
    return ret;
}

// TODO: wrapped funcs should be there. every single one.
GLINAPI void* EXPORT GLIN_GetProcAddress(const char* name)
{
// Main
    GLIN_MAP(glGetString);
// Textures
    GLIN_ALL(glGenTextures);
    GLIN_ALL(glDeleteTextures);
    GLIN_ALL(glTexImage2D);
    GLIN_ALL(glTexSubImage2D);
    GLIN_ALL(glGetTexImage);
    GLIN_ALL(glCompressedTexImage2D);
    GLIN_ALL(glTexImage2DMultisample);
    GLIN_ALL(glTexImage3DMultisample);
    GLIN_ALL(glFramebufferTexture3D);
    GLIN_ALL(glActiveTexture);
    GLIN_ALL(glBindMultiTexture);
// Render
    GLIN_MAP(glBegin);
    GLIN_MAP(glEnd);
    GLIN_MAP(glColor3f);
    GLIN_MAP(glColor4f);
    GLIN_MAP(glColor4sv);
    GLIN_MAP(glVertex3f);
    GLIN_MAP(glVertex3i);
    GLIN_MAP(glVertex2f);
    GLIN_MAP(glVertex2i);
    GLIN_MAP(glTexCoord2f);
    GLIN_MAP(glClipPlane);
    GLIN_MAP(glClipPlanef);
    GLIN_MAP(glVertexAttrib3d);
    GLIN_MAP(glMultiDrawArrays);
    GLIN_MAP(glMultiDrawElements);
    GLIN_MAP(glMultiDrawElementsBaseVertex);
    GLIN_MAP(glShadeModel);
    GLIN_MAP(glEnableClientState);
    GLIN_MAP(glDisableClientState);
    GLIN_MAP(glVertexPointer);
    GLIN_MAP(glColorPointer);
    GLIN_MAP(glTexCoordPointer);
    GLIN_MAP(glNormalPointer);
    GLIN_MAP(glLightf);
    GLIN_MAP(glLightfv);
    GLIN_MAP(glLightModelf);
    GLIN_MAP(glLightModelfv);
    GLIN_MAP(glMaterialf);
    GLIN_MAP(glMaterialfv);
    GLIN_MAP(glFogf);
    GLIN_MAP(glFogfv);
    GLIN_MAP(glFogi);
// Shader
    GLIN_ALL(glCompileShader);
    GLIN_ALL(glCreateShader);
    GLIN_ALL(glLinkProgram);
    GLIN_ALL(glGetActiveUniformName);
    GLIN_ALL(glUseProgram);
    AS_GLIN_ALL(glUseProgramObject, glUseProgram);
// Object
    GLIN_ALL(glGetInfoLog);
    GLIN_ALL(glDeleteObject);
    GLIN_ALL(glGetObjectParameterfv);
    GLIN_ALL(glGetObjectParameteriv);
// Queries (GL_EXT_disjoint_timer_query seems to be everywhere)
    AS_ALL(glGenQueries, glGenQueriesEXT);
    AS_ALL(glDeleteQueries, glDeleteQueriesEXT);
    AS_ALL(glIsQuery, glIsQueryEXT);
    AS_ALL(glBeginQuery, glBeginQueryEXT);
    AS_ALL(glEndQuery, glEndQueryEXT);
    AS_ALL(glQueryCounter, glQueryCounterEXT);
    AS_ALL(glGetQueryiv, glGetQueryivEXT);
    AS_ALL(glGetQueryObjectiv, glGetQueryObjectivEXT);
    AS_ALL(glGetQueryObjectuiv, glGetQueryObjectuivEXT);
    AS_MAP(glGetQueryObjecti64v, glGetQueryObjecti64vEXT);
    AS_MAP(glGetQueryObjectui64v, glGetQueryObjectui64vEXT);
// Programs
    GLIN_ARB(glGenPrograms);
    GLIN_ARB(glDeletePrograms);
// Buffers
    GLIN_ALL(glMapBuffer);
    GLIN_ALL(glDrawBuffer);
    GLIN_ALL(glGetBufferSubData);
    GLIN_ALL(glBindFramebuffer);
    GLIN_ALL(glCheckFramebufferStatus);
// Matrix
    GLIN_MAP(glMatrixMode);
    GLIN_MAP(glLoadIdentity);
    GLIN_MAP(glPushMatrix);
    GLIN_MAP(glPopMatrix);
    GLIN_MAP(glLoadMatrixf);
    GLIN_MAP(glLoadMatrixd);
    GLIN_MAP(glMultMatrixf);
    GLIN_MAP(glMultMatrixd);
    GLIN_MAP(glTranslatef);
    GLIN_MAP(glTranslated);
    GLIN_MAP(glScalef);
    GLIN_MAP(glScaled);
    GLIN_MAP(glRotatef);
    GLIN_MAP(glRotated);
    GLIN_MAP(glFrustum);
    GLIN_MAP(glFrustumf);
    GLIN_MAP(glOrtho);
    GLIN_MAP(glOrthof);
    GLIN_MAP(glLoadTransposeMatrixf);
    GLIN_MAP(glLoadTransposeMatrixd);
    GLIN_MAP(glMultTransposeMatrixf);
    GLIN_MAP(glMultTransposeMatrixd);
// Probably complete
    GLIN_ARB(glBindProgram);
    GLIN_ARB(glProgramString);
    GLIN_ARB(glGetProgramString);
    GLIN_ALL(glGetFloatv);
    GLIN_ALL(glGetDoublev);
    GLIN_ALL(glPixelStoref);
// Incomplete
    GLIN_ALL(glGetCompressedTexImage);
// Not implemented
    GLIN_ALL(glPolygonMode);
    GLIN_ALL(glPopAttrib);
    GLIN_ALL(glPushAttrib);
    GLIN_ALL(glClientActiveTexture);
    GLIN_ALL(glProgramEnvParameters4fv);
    GLIN_ALL(glEnableIndexed);
    GLIN_ALL(glDisableIndexed);
    GLIN_ALL(glGetBooleanIndexedv);
    GLIN_ALL(glPushClientAttrib);
    GLIN_ALL(glPopClientAttrib);
    GLIN_ALL(glLogicOp);
// Wraps (debug purpose)
    //GLIN_ALL(glClearColor);
// Default ES 3.x functions
    #include "ES3_Funcs.inl"
// Unknown function?
    MSG("GLIN_GetProcAddress(\"%s\") returned NULL!", name);
    return NULL;
}

// If we're a replacement for GL4ES:
GLINAPI void* EXPORT gl4es_GetProcAddress(const char* name) { return GLIN_GetProcAddress(name); }
GLINAPI void EXPORT initialize_gl4es() { DBG("GLinES doesn`t need to be initialized!"); }
GLINAPI void EXPORT set_getprocaddress(getprocaddressType new_proc_address) { GLIN_SetProcAddr(new_proc_address); }
GLINAPI getprocaddressType EXPORT get_getprocaddress() { return pGetProcAddr; } // for fun!

// Our own things
static glin_globals_t globalsLocal;
glin_globals_t* globals = &globalsLocal;
