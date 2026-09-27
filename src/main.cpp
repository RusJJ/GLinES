#include "GLES.h"
#include <dlfcn.h>
#include <sstream>
#include <string>

#include "wrapped.h"
#include "gl_buffer.h"
#include "gl_matrix.h"
#include "gl_object.h"
#include "gl_queries.h"
#include "gl_render.h"
#include "gl_shader.h"
#include "gl_texture.h"
#if defined(SYS64)
    #define RET_CMP(__fn_name, __fn) if(strcmp(name, __fn_name) == 0) { DBG("GLinES returned 0x%016llX for %s", (unsigned long long)__fn, __fn_name); return (void*)__fn; }
#else
    #define RET_CMP(__fn_name, __fn) if(strcmp(name, __fn_name) == 0) { DBG("GLinES returned 0x%08X for %s", (unsigned int)__fn, __fn_name); return (void*)__fn; }
#endif
#define GL_MAP(__fn_name) if(strcmp(name, #__fn_name) == 0) return GLIN_GetBackendProc(#__fn_name);
#define GL_ARB(__fn_name) if(strcmp(name, #__fn_name "ARB") == 0) return GLIN_GetBackendProc(#__fn_name);
#define GL_EXT(__fn_name) if(strcmp(name, #__fn_name "EXT") == 0) return GLIN_GetBackendProc(#__fn_name);
#define GL_ALL(__fn_name)                       GL_MAP(__fn_name); GL_ARB(__fn_name); GL_EXT(__fn_name)
#define AS_MAP(__fn_name, __ret_fn_name) if(strcmp(name, #__fn_name "") == 0) return GLIN_GetBackendProc(#__ret_fn_name);
#define AS_ARB(__fn_name, __ret_fn_name) if(strcmp(name, #__fn_name "ARB") == 0) return GLIN_GetBackendProc(#__ret_fn_name);
#define AS_EXT(__fn_name, __ret_fn_name) if(strcmp(name, #__fn_name "EXT") == 0) return GLIN_GetBackendProc(#__ret_fn_name);
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
const char* pszGLExtensions =
    #include "GL_Exts.inl"
;
static const std::vector<std::string>& ExtensionNames()
{
    static const std::vector<std::string> names = []() {
        std::vector<std::string> result;
        std::istringstream stream(pszGLExtensions);
        std::string name;
        while(stream >> name) result.push_back(name);
        return result;
    }();
    return names;
}

const GLubyte* WRAP(glGetStringi(GLenum name, GLuint index))
{
    if(name != GL_EXTENSIONS) { SetError(GL_INVALID_ENUM); return nullptr; }
    const auto& names = ExtensionNames();
    if(index >= names.size()) { SetError(GL_INVALID_VALUE); return nullptr; }
    return (const GLubyte*)names[index].c_str();
}

GLint GLIN_ExtensionCount() { return (GLint)ExtensionNames().size(); }
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
            return (GLubyte*)"3.3";

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

GLINAPI EXPORT void* GLIN_ProcAddr(void* lib, const char* name)
{
    void* ret = NULL;
    if(pGetProcAddr != NULL) ret = pGetProcAddr(name);
    if(ret == NULL) ret = dlsym(lib, name);
    return ret;
}

GLINAPI void* GLIN_GetBackendProc(const char* name)
{
    if(!name) return nullptr;
    static void* library = []() {
        void* handle = dlopen("libGLESv3.so", RTLD_NOW | RTLD_LOCAL);
        if(!handle) handle = dlopen("libGLESv2.so.2", RTLD_NOW | RTLD_LOCAL);
        if(!handle) handle = dlopen("libGLESv2.so", RTLD_NOW | RTLD_LOCAL);
        return handle;
    }();
    if(void* result = library ? dlsym(library, name) : nullptr) return result;
    static void* egl = []() {
        void* handle = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
        return handle ? handle : dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL);
    }();
    auto getProc = egl ? (void*(*)(const char*))dlsym(egl, "eglGetProcAddress") : nullptr;
    return getProc ? getProc(name) : nullptr;
}

GLenum WRAP(glGetError())
{
    GLenum error = globals->error;
    globals->error = GL_NO_ERROR;
    return error == GL_NO_ERROR ? glGetError() : error;
}

GLINAPI EXPORT void* GLIN_GetProcAddress(const char* name)
{
    if(!name) return nullptr;
    GLIN_MAP(glGetError);
    GLIN_ALL(glUniformMatrix2fv);
    GLIN_ALL(glUniformMatrix3fv);
    GLIN_ALL(glUniformMatrix4fv);
    GLIN_ALL(glUniformMatrix2x3fv);
    GLIN_ALL(glUniformMatrix3x2fv);
    GLIN_ALL(glUniformMatrix2x4fv);
    GLIN_ALL(glUniformMatrix4x2fv);
    GLIN_ALL(glUniformMatrix3x4fv);
    GLIN_ALL(glUniformMatrix4x3fv);

    GLIN_MAP(glGetStringi);
    GLIN_MAP(glDeleteShader);
    GLIN_ALL(glShaderSource);
    GLIN_MAP(glGetShaderSource);
    GLIN_MAP(glTexParameteri);
    GLIN_MAP(glTexParameterf);
    GLIN_MAP(glTexParameteriv);
    GLIN_MAP(glTexParameterfv);
    GLIN_MAP(glGetTexParameteriv);
    GLIN_MAP(glGetTexParameterfv);
    GLIN_MAP(glTexStorage2D);
    GLIN_MAP(glBindTextureUnit);
// -----------------------------------------------------------------------
    GLIN_MAP(glGetString);
    GLIN_ALL(glGetBooleanv);
    GLIN_ALL(glGetIntegerv);
    GLIN_ALL(glGetFloatv);
    GLIN_ALL(glGetDoublev);
    GLIN_ALL(glEnable);
    GLIN_ALL(glDisable);
    GLIN_ALL(glIsEnabled);
// -----------------------------------------------------------------------
    GLIN_MAP(glGetLightfv);
    GLIN_MAP(glGetLightiv);
    GLIN_MAP(glGetMaterialfv);
    GLIN_MAP(glGetMaterialiv);
    GLIN_MAP(glGetClipPlane);
    GLIN_MAP(glGetTexEnvfv);
    GLIN_MAP(glGetTexEnviv);
    GLIN_MAP(glGetTexGenfv);
    GLIN_MAP(glGetTexGendv);
    GLIN_MAP(glGetTexGeniv);
// -----------------------------------------------------------------------
    GLIN_ALL(glGenTextures);
    GLIN_ALL(glDeleteTextures);
    GLIN_ALL(glBindTexture);
    GLIN_ALL(glTexImage2D);
    GLIN_ALL(glTexSubImage2D);
    GLIN_ALL(glGetTexImage);
    GLIN_ALL(glCompressedTexImage2D);
    GLIN_ALL(glTexImage2DMultisample);
    GLIN_ALL(glTexImage3DMultisample);
    GLIN_ALL(glFramebufferTexture3D);
    GLIN_ALL(glActiveTexture);
    GLIN_ALL(glBindMultiTexture);
    // 1D remapped to 2D
    GLIN_MAP(glTexImage1D);
    GLIN_MAP(glTexSubImage1D);
    GLIN_MAP(glCopyTexImage1D);
    GLIN_MAP(glCopyTexSubImage1D);
    GLIN_MAP(glCopyTexImage2D);
    GLIN_MAP(glCopyTexSubImage2D);
    GLIN_MAP(glFramebufferTexture1D);
    // Texture residency (GL 1.1)
    GLIN_MAP(glPrioritizeTextures);
    GLIN_MAP(glAreTexturesResident);
// -----------------------------------------------------------------------
    GLIN_MAP(glBegin);
    GLIN_MAP(glEnd);
// -----------------------------------------------------------------------
    GLIN_MAP(glColor3f);  GLIN_MAP(glColor3b);  GLIN_MAP(glColor3d);
    GLIN_MAP(glColor3i);  GLIN_MAP(glColor3s);  GLIN_MAP(glColor3ub);
    GLIN_MAP(glColor3ui); GLIN_MAP(glColor3us);
    GLIN_MAP(glColor3bv); GLIN_MAP(glColor3dv); GLIN_MAP(glColor3fv);
    GLIN_MAP(glColor3iv); GLIN_MAP(glColor3sv); GLIN_MAP(glColor3ubv);
    GLIN_MAP(glColor3uiv);GLIN_MAP(glColor3usv);
    GLIN_MAP(glColor4f);  GLIN_MAP(glColor4b);  GLIN_MAP(glColor4d);
    GLIN_MAP(glColor4i);  GLIN_MAP(glColor4s);  GLIN_MAP(glColor4ub);
    GLIN_MAP(glColor4ui); GLIN_MAP(glColor4us);
    GLIN_MAP(glColor4bv); GLIN_MAP(glColor4dv); GLIN_MAP(glColor4fv);
    GLIN_MAP(glColor4iv); GLIN_MAP(glColor4sv); GLIN_MAP(glColor4ubv);
    GLIN_MAP(glColor4uiv);GLIN_MAP(glColor4usv);
// -----------------------------------------------------------------------
    GLIN_MAP(glNormal3f); GLIN_MAP(glNormal3b); GLIN_MAP(glNormal3d);
    GLIN_MAP(glNormal3i); GLIN_MAP(glNormal3s);
    GLIN_MAP(glNormal3bv);GLIN_MAP(glNormal3dv);GLIN_MAP(glNormal3fv);
    GLIN_MAP(glNormal3iv);GLIN_MAP(glNormal3sv);
// -----------------------------------------------------------------------
    GLIN_MAP(glVertex2f); GLIN_MAP(glVertex2i); GLIN_MAP(glVertex2d); GLIN_MAP(glVertex2s);
    GLIN_MAP(glVertex3f); GLIN_MAP(glVertex3i); GLIN_MAP(glVertex3d); GLIN_MAP(glVertex3s);
    GLIN_MAP(glVertex4f); GLIN_MAP(glVertex4i); GLIN_MAP(glVertex4d); GLIN_MAP(glVertex4s);
    GLIN_MAP(glVertex2fv);GLIN_MAP(glVertex2dv);GLIN_MAP(glVertex2iv);GLIN_MAP(glVertex2sv);
    GLIN_MAP(glVertex3fv);GLIN_MAP(glVertex3dv);GLIN_MAP(glVertex3iv);GLIN_MAP(glVertex3sv);
    GLIN_MAP(glVertex4fv);GLIN_MAP(glVertex4dv);GLIN_MAP(glVertex4iv);GLIN_MAP(glVertex4sv);
// -----------------------------------------------------------------------
    GLIN_MAP(glTexCoord1f);GLIN_MAP(glTexCoord1d);GLIN_MAP(glTexCoord1i);GLIN_MAP(glTexCoord1s);
    GLIN_MAP(glTexCoord2f);GLIN_MAP(glTexCoord2d);GLIN_MAP(glTexCoord2i);GLIN_MAP(glTexCoord2s);
    GLIN_MAP(glTexCoord3f);GLIN_MAP(glTexCoord3d);GLIN_MAP(glTexCoord3i);GLIN_MAP(glTexCoord3s);
    GLIN_MAP(glTexCoord4f);GLIN_MAP(glTexCoord4d);GLIN_MAP(glTexCoord4i);GLIN_MAP(glTexCoord4s);
    GLIN_MAP(glTexCoord1fv);GLIN_MAP(glTexCoord2fv);GLIN_MAP(glTexCoord2dv);
    GLIN_MAP(glTexCoord2iv);GLIN_MAP(glTexCoord2sv);GLIN_MAP(glTexCoord3fv);GLIN_MAP(glTexCoord4fv);
// -----------------------------------------------------------------------
    GLIN_ALL(glMultiTexCoord1f);  GLIN_ALL(glMultiTexCoord2f);
    GLIN_ALL(glMultiTexCoord3f);  GLIN_ALL(glMultiTexCoord4f);
    GLIN_ALL(glMultiTexCoord1d);  GLIN_ALL(glMultiTexCoord2d);
    GLIN_ALL(glMultiTexCoord3d);  GLIN_ALL(glMultiTexCoord4d);
    GLIN_ALL(glMultiTexCoord1i);  GLIN_ALL(glMultiTexCoord2i);
    GLIN_ALL(glMultiTexCoord2s);
    GLIN_ALL(glMultiTexCoord1fv); GLIN_ALL(glMultiTexCoord2fv);
    GLIN_ALL(glMultiTexCoord3fv); GLIN_ALL(glMultiTexCoord4fv);
    GLIN_ALL(glMultiTexCoord2dv); GLIN_ALL(glMultiTexCoord2iv);
    GLIN_ALL(glMultiTexCoord2sv);
// -----------------------------------------------------------------------
    GLIN_MAP(glTexGenf); GLIN_MAP(glTexGend); GLIN_MAP(glTexGeni);
    GLIN_MAP(glTexGenfv);GLIN_MAP(glTexGendv);GLIN_MAP(glTexGeniv);
// -----------------------------------------------------------------------
    GLIN_ALL(glDrawArrays);
    GLIN_ALL(glDrawElements);
    GLIN_ALL(glDrawRangeElements);
    GLIN_ALL(glDrawElementsBaseVertex);
    GLIN_ALL(glPrimitiveRestartIndex);
    GLIN_ALL(glDrawElementsInstanced);
    GLIN_ALL(glDrawElementsInstancedBaseVertex);
    GLIN_MAP(glMultiDrawArrays);
    GLIN_MAP(glMultiDrawElements);
    GLIN_MAP(glMultiDrawElementsBaseVertex);
    GLIN_MAP(glArrayElement);
    GLIN_MAP(glInterleavedArrays);
// -----------------------------------------------------------------------
    GLIN_MAP(glEnableClientState);
    GLIN_MAP(glDisableClientState);
    GLIN_MAP(glVertexPointer);
    GLIN_MAP(glColorPointer);
    GLIN_MAP(glTexCoordPointer);
    GLIN_MAP(glNormalPointer);
    GLIN_MAP(glSecondaryColorPointer);
    GLIN_MAP(glEdgeFlagPointer);
    GLIN_MAP(glClientActiveTexture);
// -----------------------------------------------------------------------
    GLIN_MAP(glClipPlane);
    GLIN_MAP(glClipPlanef);
    GLIN_MAP(glGetClipPlane);
// -----------------------------------------------------------------------
    GLIN_MAP(glClearDepth);
    GLIN_MAP(glDepthRange);
// -----------------------------------------------------------------------
    GLIN_MAP(glLightf);     GLIN_MAP(glLightfv);
    GLIN_MAP(glLightModelf);GLIN_MAP(glLightModelfv);GLIN_MAP(glLightModeli);
    GLIN_MAP(glGetLightfv); GLIN_MAP(glGetLightiv);
// -----------------------------------------------------------------------
    GLIN_MAP(glMaterialf); GLIN_MAP(glMaterialfv);
    GLIN_MAP(glMateriali); GLIN_MAP(glMaterialiv);
    GLIN_MAP(glGetMaterialfv);GLIN_MAP(glGetMaterialiv);
    GLIN_MAP(glColorMaterial);
// -----------------------------------------------------------------------
    GLIN_MAP(glFogf); GLIN_MAP(glFogfv); GLIN_MAP(glFogi);
// -----------------------------------------------------------------------
    GLIN_MAP(glShadeModel);
    GLIN_ALL(glPolygonMode);
    GLIN_MAP(glLogicOp);
// -----------------------------------------------------------------------
    GLIN_MAP(glTexEnvi);  GLIN_MAP(glTexEnvf);
    GLIN_MAP(glTexEnvfv); GLIN_MAP(glTexEnviv);
    GLIN_MAP(glGetTexEnvfv);GLIN_MAP(glGetTexEnviv);
// -----------------------------------------------------------------------
    GLIN_MAP(glAlphaFunc);
    GLIN_MAP(glPointSize);
    GLIN_MAP(glLineWidth);
    GLIN_ALL(glPointParameterf);
    GLIN_ALL(glPointParameteri);
    GLIN_ALL(glPointParameterfv);
    GLIN_ALL(glPointParameteriv);
// -----------------------------------------------------------------------
    GLIN_MAP(glVertexAttrib3d);
    GLIN_MAP(glVertexAttribLPointer);
// -----------------------------------------------------------------------
    GLIN_MAP(glEdgeFlag);
    GLIN_MAP(glEdgeFlagv);
// -----------------------------------------------------------------------
    GLIN_MAP(glRectf); GLIN_MAP(glRectd); GLIN_MAP(glRecti); GLIN_MAP(glRects);
    GLIN_MAP(glRectfv);GLIN_MAP(glRectdv);GLIN_MAP(glRectiv);GLIN_MAP(glRectsv);
// -----------------------------------------------------------------------
    GLIN_MAP(glRasterPos2f);GLIN_MAP(glRasterPos2d);GLIN_MAP(glRasterPos2i);GLIN_MAP(glRasterPos2s);
    GLIN_MAP(glRasterPos3f);GLIN_MAP(glRasterPos3d);GLIN_MAP(glRasterPos3i);GLIN_MAP(glRasterPos3s);
    GLIN_MAP(glRasterPos4f);GLIN_MAP(glRasterPos4d);GLIN_MAP(glRasterPos4i);GLIN_MAP(glRasterPos4s);
    GLIN_MAP(glRasterPos2fv);GLIN_MAP(glRasterPos2dv);GLIN_MAP(glRasterPos2iv);
    GLIN_MAP(glRasterPos3fv);GLIN_MAP(glRasterPos3dv);
    GLIN_MAP(glRasterPos4fv);GLIN_MAP(glRasterPos4dv);
// -----------------------------------------------------------------------
    GLIN_ALL(glWindowPos2f);GLIN_ALL(glWindowPos2d);GLIN_ALL(glWindowPos2i);
    GLIN_ALL(glWindowPos3f);GLIN_ALL(glWindowPos3d);
    GLIN_MAP(glWindowPos2fv);GLIN_MAP(glWindowPos2iv);GLIN_MAP(glWindowPos3fv);
// -----------------------------------------------------------------------
    GLIN_MAP(glBitmap);
    GLIN_MAP(glDrawPixels);
    GLIN_MAP(glCopyPixels);
    GLIN_MAP(glPixelZoom);
    GLIN_MAP(glPixelTransferf);
    GLIN_MAP(glPixelTransferi);
    GLIN_MAP(glPixelMapfv);
    GLIN_MAP(glPixelMapuiv);
    GLIN_MAP(glPixelMapusv);
    GLIN_MAP(glGetPixelMapfv);
    GLIN_MAP(glGetPixelMapuiv);
    GLIN_MAP(glGetPixelMapusv);
// -----------------------------------------------------------------------
    GLIN_MAP(glPolygonStipple);
    GLIN_MAP(glGetPolygonStipple);
    GLIN_MAP(glLineStipple);
// -----------------------------------------------------------------------
    GLIN_ALL(glSecondaryColor3f); GLIN_ALL(glSecondaryColor3d);
    GLIN_ALL(glSecondaryColor3ub);GLIN_ALL(glSecondaryColor3fv);
    GLIN_ALL(glSecondaryColor3dv);
    GLIN_ALL(glFogCoordf);GLIN_ALL(glFogCoordd);
    GLIN_ALL(glFogCoordfv);GLIN_ALL(glFogCoorddv);
// -----------------------------------------------------------------------
    GLIN_MAP(glGenLists);
    GLIN_MAP(glNewList);
    GLIN_MAP(glEndList);
    GLIN_MAP(glCallList);
    GLIN_MAP(glCallLists);
    GLIN_MAP(glDeleteLists);
    GLIN_MAP(glIsList);
    GLIN_MAP(glListBase);
// -----------------------------------------------------------------------
    GLIN_ALL(glPushAttrib);
    GLIN_ALL(glPopAttrib);
    GLIN_ALL(glPushClientAttrib);
    GLIN_ALL(glPopClientAttrib);
// -----------------------------------------------------------------------
    GLIN_ALL(glCompileShader);
    GLIN_ALL(glCreateShader);
    GLIN_ALL(glLinkProgram);
    GLIN_ALL(glGetActiveUniformName);
    GLIN_ALL(glUseProgram);
    AS_GLIN_ALL(glUseProgramObject, glUseProgram);
// -----------------------------------------------------------------------
    GLIN_ALL(glGetInfoLog);
    GLIN_ALL(glDeleteObject);
    GLIN_ALL(glGetObjectParameterfv);
    GLIN_ALL(glGetObjectParameteriv);
// -----------------------------------------------------------------------
    GLIN_ALL(glGenQueries);
    GLIN_ALL(glDeleteQueries);
    GLIN_ALL(glIsQuery);
    GLIN_ALL(glBeginQuery);
    GLIN_ALL(glEndQuery);
    GLIN_ALL(glQueryCounter);
    GLIN_ALL(glGetQueryiv);
    GLIN_ALL(glGetQueryObjectiv);
    GLIN_ALL(glGetQueryObjectuiv);
    GLIN_ALL(glGetQueryObjecti64v);
    GLIN_ALL(glGetQueryObjectui64v);
// -----------------------------------------------------------------------
    GLIN_ARB(glGenPrograms);
    GLIN_ARB(glDeletePrograms);
    GLIN_ARB(glBindProgram);
    GLIN_ARB(glProgramString);
    GLIN_ARB(glGetProgramString);
    GLIN_ARB(glGetProgramiv); // ARB program state queries
// -----------------------------------------------------------------------
    GLIN_ALL(glMapBuffer);
    GLIN_ALL(glDrawBuffer);
    GLIN_ALL(glGetBufferSubData);
    GLIN_ALL(glBindFramebuffer);
    GLIN_ALL(glCheckFramebufferStatus);
    GLIN_ALL(glBindBuffer);
    GLIN_ALL(glBindBufferRange);
    GLIN_ALL(glBindBufferBase);
// -----------------------------------------------------------------------
    GLIN_MAP(glMatrixMode);
    GLIN_MAP(glLoadIdentity);
    GLIN_MAP(glPushMatrix);
    GLIN_MAP(glPopMatrix);
    GLIN_MAP(glLoadMatrixf);    GLIN_MAP(glLoadMatrixd);
    GLIN_MAP(glMultMatrixf);    GLIN_MAP(glMultMatrixd);
    GLIN_MAP(glTranslatef);     GLIN_MAP(glTranslated);
    GLIN_MAP(glScalef);         GLIN_MAP(glScaled);
    GLIN_MAP(glRotatef);        GLIN_MAP(glRotated);
    GLIN_MAP(glFrustum);        GLIN_MAP(glFrustumf);
    GLIN_MAP(glOrtho);          GLIN_MAP(glOrthof);
    GLIN_MAP(glLoadTransposeMatrixf); GLIN_MAP(glLoadTransposeMatrixd);
    GLIN_MAP(glMultTransposeMatrixf); GLIN_MAP(glMultTransposeMatrixd);
// -----------------------------------------------------------------------
    GLIN_ALL(glPixelStoref);
// -----------------------------------------------------------------------
    GLIN_ALL(glHint);
// -----------------------------------------------------------------------
    GLIN_ALL(glGetCompressedTexImage);
// -----------------------------------------------------------------------
    GLIN_MAP(glDrawRangeElementsBaseVertex);
// -----------------------------------------------------------------------
    GLIN_MAP(glAccum);
    GLIN_MAP(glClearAccum);
    GLIN_MAP(glPassThrough);
// -----------------------------------------------------------------------
    GLIN_MAP(glSelectBuffer);
    GLIN_MAP(glFeedbackBuffer);
    GLIN_MAP(glRenderMode);
    GLIN_MAP(glInitNames);
    GLIN_MAP(glPushName);
    GLIN_MAP(glPopName);
    GLIN_MAP(glLoadName);
// -----------------------------------------------------------------------
    GLIN_MAP(glMap1f);    GLIN_MAP(glMap1d);
    GLIN_MAP(glMap2f);    GLIN_MAP(glMap2d);
    GLIN_MAP(glMapGrid1f);GLIN_MAP(glMapGrid1d);
    GLIN_MAP(glMapGrid2f);GLIN_MAP(glMapGrid2d);
    GLIN_MAP(glEvalMesh1);GLIN_MAP(glEvalMesh2);
    GLIN_MAP(glEvalPoint1);GLIN_MAP(glEvalPoint2);
    GLIN_MAP(glEvalCoord1f);GLIN_MAP(glEvalCoord1d);
    GLIN_MAP(glEvalCoord2f);GLIN_MAP(glEvalCoord2d);
    GLIN_MAP(glGetMapiv);GLIN_MAP(glGetMapfv);GLIN_MAP(glGetMapdv);
// -----------------------------------------------------------------------
    GLIN_MAP(glColorTable);
    GLIN_MAP(glColorTableParameterfv);GLIN_MAP(glColorTableParameteriv);
    GLIN_MAP(glCopyColorTable);
    GLIN_MAP(glGetColorTable);
    GLIN_MAP(glGetColorTableParameterfv);GLIN_MAP(glGetColorTableParameteriv);
    GLIN_MAP(glColorSubTable);GLIN_MAP(glCopyColorSubTable);
    GLIN_MAP(glConvolutionFilter1D);GLIN_MAP(glConvolutionFilter2D);
    GLIN_MAP(glConvolutionParameterf);GLIN_MAP(glConvolutionParameterfv);
    GLIN_MAP(glConvolutionParameteri);GLIN_MAP(glConvolutionParameteriv);
    GLIN_MAP(glCopyConvolutionFilter1D);GLIN_MAP(glCopyConvolutionFilter2D);
    GLIN_MAP(glGetConvolutionFilter);
    GLIN_MAP(glGetConvolutionParameterfv);GLIN_MAP(glGetConvolutionParameteriv);
    GLIN_MAP(glSeparableFilter2D);GLIN_MAP(glGetSeparableFilter);
    GLIN_MAP(glHistogram);GLIN_MAP(glResetHistogram);GLIN_MAP(glGetHistogram);
    GLIN_MAP(glGetHistogramParameterfv);GLIN_MAP(glGetHistogramParameteriv);
    GLIN_MAP(glMinmax);GLIN_MAP(glResetMinmax);GLIN_MAP(glGetMinmax);
    GLIN_MAP(glGetMinmaxParameterfv);GLIN_MAP(glGetMinmaxParameteriv);
// -----------------------------------------------------------------------
    GLIN_MAP(glClampColor);
// -----------------------------------------------------------------------
    GLIN_ALL(glProgramEnvParameters4fv);
    GLIN_ALL(glEnableIndexed);
    GLIN_ALL(glDisableIndexed);
    GLIN_ALL(glGetBooleanIndexedv);
// -----------------------------------------------------------------------
    GLIN_MAP(glPrioritizeTextures);
    GLIN_MAP(glAreTexturesResident);
// -----------------------------------------------------------------------
    #include "ES3_Funcs.inl"
// -----------------------------------------------------------------------
    MSG("GLIN_GetProcAddress(\"%s\") returned NULL!", name);
    return NULL;
}

// GL4ES compatibility shim
GLINAPI EXPORT void* gl4es_GetProcAddress(const char* name) { return GLIN_GetProcAddress(name); }
GLINAPI void EXPORT initialize_gl4es() { DBG("GLinES doesn`t need to be initialized!"); }
GLINAPI void EXPORT set_getprocaddress(getprocaddressType new_proc_address) { GLIN_SetProcAddr(new_proc_address); }
GLINAPI getprocaddressType EXPORT get_getprocaddress() { return pGetProcAddr; }

// Global state
thread_local glin_globals_t globalsLocal;
thread_local glin_globals_t* globals = &globalsLocal;
