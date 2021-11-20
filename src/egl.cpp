#include "GLES.h"
#include <EGL/egl.h>
#include <dlfcn.h>

#if defined(SYS64)
    #define RET_CMP(__fn_name, __fn) if(strcmp(name, __fn_name) == 0) { DBG("GLinES returned 0x%LX for %s", (unsigned long long)__fn, __fn_name); return (void(*)())__fn; }
#else
    #define RET_CMP(__fn_name, __fn) if(strcmp(name, __fn_name) == 0) { DBG("GLinES returned 0x%X for %s", (unsigned int)__fn, __fn_name); return (void(*)())__fn; }
#endif
#define EGL_MAP(__fn_name)           RET_CMP(#__fn_name,       __fn_name)

GLINAPI void* EXPORT GLIN_GetProcAddress(const char* name);
EGLBoolean (*pEGL_BindAPI) (EGLenum api);
EGLBoolean (*pEGL_ChooseConfig) (EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
EGLContext (*pEGL_CreateContext) (EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list);
EGLSurface (*pEGL_CreateWindowSurface) (EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list);
EGLBoolean (*pEGL_DestroyContext) (EGLDisplay dpy, EGLContext ctx);
EGLBoolean (*pEGL_DestroySurface) (EGLDisplay dpy, EGLSurface surface);
EGLContext (*pEGL_GetCurrentContext) (void);
EGLSurface (*pEGL_GetCurrentSurface) (EGLint readdraw);
EGLBoolean (*pEGL_GetConfigAttrib) (EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
EGLDisplay (*pEGL_GetDisplay) (NativeDisplayType display);
EGLDisplay (*pEGL_GetPlatformDisplay) (EGLenum platform, void *native_display, const EGLAttrib *attrib_list);
EGLint     (*pEGL_GetError) (void);
EGLBoolean (*pEGL_Initialize) (EGLDisplay dpy, EGLint *major, EGLint *minor);
EGLBoolean (*pEGL_MakeCurrent) (EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
EGLBoolean (*pEGL_ReleaseThread) (void);
EGLBoolean (*pEGL_SwapBuffers) (EGLDisplay dpy, EGLSurface draw);
EGLBoolean (*pEGL_SwapInterval) (EGLDisplay dpy, EGLint interval);
EGLBoolean (*pEGL_Terminate) (EGLDisplay dpy);

static void* StaticEGLInit()
{
    DBG("Getting EGL Library handle...");
    void* ret = dlopen("libEGL.so", RTLD_NOW);
    if( !ret )
    {
        ERR("I dont have an EGL!");
        return NULL;
    }

    pEGL_BindAPI = (EGLBoolean(*)(EGLenum))dlsym(ret, "eglBindAPI");
    pEGL_ChooseConfig = (EGLBoolean(*)(EGLDisplay, const EGLint*, EGLConfig*, EGLint, EGLint*))dlsym(ret, "eglChooseConfig");
    pEGL_CreateContext = (EGLContext(*)(EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list))dlsym(ret, "eglCreateContext");
    pEGL_CreateWindowSurface = (EGLSurface(*)(EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list))dlsym(ret, "eglCreateWindowSurface");
    pEGL_DestroyContext = (EGLBoolean(*)(EGLDisplay dpy, EGLContext ctx))dlsym(ret, "eglDestroyContext");
    pEGL_DestroySurface = (EGLBoolean(*)(EGLDisplay dpy, EGLSurface surface))dlsym(ret, "eglDestroySurface");
    pEGL_GetCurrentContext = (EGLContext(*)(void))dlsym(ret, "eglGetCurrentContext");
    pEGL_GetCurrentSurface = (EGLSurface(*)(EGLint readdraw))dlsym(ret, "eglGetCurrentSurface");
    pEGL_GetConfigAttrib = (EGLBoolean(*)(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value))dlsym(ret, "eglGetConfigAttrib");
    pEGL_GetDisplay = (EGLDisplay(*)(NativeDisplayType display))dlsym(ret, "eglGetDisplay");
    pEGL_GetPlatformDisplay = (EGLDisplay(*)(EGLenum platform, void *native_display, const EGLAttrib *attrib_list))dlsym(ret, "eglGetPlatformDisplay");
    pEGL_GetError = (EGLint(*)(void))dlsym(ret, "eglGetError");
    pEGL_Initialize = (EGLBoolean(*)(EGLDisplay dpy, EGLint *major, EGLint *minor))dlsym(ret, "eglInitialize");
    pEGL_MakeCurrent = (EGLBoolean(*)(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx))dlsym(ret, "eglMakeCurrent");
    pEGL_ReleaseThread = (EGLBoolean(*)(void))dlsym(ret, "eglReleaseThread");
    pEGL_SwapBuffers = (EGLBoolean(*)(EGLDisplay dpy, EGLSurface draw))dlsym(ret, "eglSwapBuffers");
    pEGL_SwapInterval = (EGLBoolean(*)(EGLDisplay dpy, EGLint interval))dlsym(ret, "eglSwapInterval");
    pEGL_Terminate = (EGLBoolean(*)(EGLDisplay dpy))dlsym(ret, "eglTerminate");
    return ret;
}
static void* eglLib = StaticEGLInit();

static EGLint egl_attrib[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 3,
    EGL_NONE
};

GLINAPI EGLBoolean EXPORT eglBindAPI(EGLenum api) { return pEGL_BindAPI(EGL_OPENGL_ES_API); }
GLINAPI EGLBoolean EXPORT eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config) { return pEGL_ChooseConfig(dpy, attrib_list, configs, config_size, num_config); }
GLINAPI EGLContext EXPORT eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list) { return pEGL_CreateContext(dpy, config, share_list, egl_attrib); }
GLINAPI EGLSurface EXPORT eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list) { return pEGL_CreateWindowSurface(dpy, config, window, attrib_list); }
GLINAPI EGLBoolean EXPORT eglDestroyContext(EGLDisplay dpy, EGLContext ctx) { return pEGL_DestroyContext(dpy, ctx); }
GLINAPI EGLBoolean EXPORT eglDestroySurface(EGLDisplay dpy, EGLSurface surface) { return pEGL_DestroySurface(dpy, surface); }
GLINAPI EGLContext EXPORT eglGetCurrentContext(void) { return pEGL_GetCurrentContext(); }
GLINAPI EGLSurface EXPORT eglGetCurrentSurface(EGLint readdraw) { return pEGL_GetCurrentSurface(readdraw); }
GLINAPI EGLBoolean EXPORT eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
    static EGLint tmp_val = 0;
    EGLBoolean ret = pEGL_GetConfigAttrib(dpy, config, attribute, &tmp_val);
    if(attribute == EGL_RENDERABLE_TYPE) tmp_val |= EGL_OPENGL_BIT;
    *value = tmp_val;
    return ret;
}
GLINAPI EGLDisplay EXPORT eglGetDisplay(NativeDisplayType display) { return pEGL_GetDisplay(display); }
GLINAPI EGLDisplay EXPORT eglGetPlatformDisplay(EGLenum platform, void *native_display, const EGLAttrib *attrib_list)
{
    if(pEGL_GetPlatformDisplay) return pEGL_GetPlatformDisplay(platform, native_display, attrib_list);
    return pEGL_GetDisplay((NativeDisplayType)native_display);
}
GLINAPI EGLint     EXPORT eglGetError(void) { return pEGL_GetError(); }
GLINAPI EGLBoolean EXPORT eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor) { return pEGL_Initialize(dpy, major, minor); }
GLINAPI EGLBoolean EXPORT eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) { return pEGL_MakeCurrent(dpy, draw, read, ctx); }
GLINAPI EGLBoolean EXPORT eglReleaseThread(void) { return pEGL_ReleaseThread(); }
GLINAPI EGLBoolean EXPORT eglSwapBuffers(EGLDisplay dpy, EGLSurface draw) { return pEGL_SwapBuffers(dpy, draw); }
GLINAPI EGLBoolean EXPORT eglSwapInterval(EGLDisplay dpy, EGLint interval) { return pEGL_SwapInterval(dpy, interval); }
GLINAPI EGLBoolean EXPORT eglTerminate(EGLDisplay dpy) { return pEGL_Terminate(dpy); }
GLINAPI EGLenum EXPORT eglQueryAPI() { return EGL_OPENGL_API; }

GLINAPI void(* EXPORT eglGetProcAddress(const char *name))(void)
{
    EGL_MAP(eglBindAPI);
    EGL_MAP(eglChooseConfig);
    EGL_MAP(eglCreateContext);
    EGL_MAP(eglCreateWindowSurface);
    EGL_MAP(eglDestroyContext);
    EGL_MAP(eglDestroySurface);
    EGL_MAP(eglGetCurrentContext);
    EGL_MAP(eglGetCurrentSurface);
    EGL_MAP(eglGetConfigAttrib);
    EGL_MAP(eglGetDisplay);
    EGL_MAP(eglGetError);
    EGL_MAP(eglInitialize);
    EGL_MAP(eglMakeCurrent);
    EGL_MAP(eglReleaseThread);
    EGL_MAP(eglSwapBuffers);
    EGL_MAP(eglSwapInterval);
    EGL_MAP(eglTerminate);
    EGL_MAP(eglQueryAPI);
    //return (void(*)())GLIN_GetProcAddress(name);
    return NULL;
}