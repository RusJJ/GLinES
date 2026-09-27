#include "GLES.h"
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <memory>
#include <mutex>
#include <dlfcn.h>

#if defined(SYS64)
    #define RET_CMP(__fn_name, __fn) if(strcmp(name, __fn_name) == 0) { DBG("GLinES returned 0x%LX for %s", (unsigned long long)__fn, __fn_name); return (void(*)())__fn; }
#else
    #define RET_CMP(__fn_name, __fn) if(strcmp(name, __fn_name) == 0) { DBG("GLinES returned 0x%X for %s", (unsigned int)__fn, __fn_name); return (void(*)())__fn; }
#endif
#define EGL_MAP(__fn_name)           RET_CMP(#__fn_name,       __fn_name)

GLINAPI EXPORT void* GLIN_GetProcAddress(const char* name);
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
const char* (*pEGL_QueryString) (EGLDisplay dpy, EGLint name);
EGLDisplay (*pEGL_GetCurrentDisplay) ();

static void* StaticEGLInit()
{
    DBG("Getting EGL Library handle...");
    void* ret = dlopen("libEGL.so", RTLD_NOW);
    if(!ret) ret = dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL);
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
    pEGL_QueryString = (const char*(*)(EGLDisplay dpy, EGLint name))dlsym(ret, "eglQueryString");
    pEGL_GetCurrentDisplay = (EGLDisplay(*)())dlsym(ret, "eglGetCurrentDisplay");
    return ret;
}
static void* eglLib = StaticEGLInit();

static std::mutex contextMutex;
static std::unordered_map<EGLContext, std::shared_ptr<glin_globals_t>> contexts;
static thread_local std::shared_ptr<glin_globals_t> currentState;
extern thread_local glin_globals_t globalsLocal;

static EGLint egl_attrib[] =
{
    EGL_CONTEXT_MAJOR_VERSION_KHR, 3,
    EGL_CONTEXT_MINOR_VERSION_KHR, 2,
    EGL_NONE
};

GLINAPI EGLBoolean EXPORT eglBindAPI(EGLenum api) { return pEGL_BindAPI ? pEGL_BindAPI(api == EGL_OPENGL_API ? EGL_OPENGL_ES_API : api) : EGL_FALSE; }
GLINAPI EGLBoolean EXPORT eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    std::vector<EGLint> attributes;
    if(attrib_list) for(const EGLint* p = attrib_list; *p != EGL_NONE; p += 2)
    {
        EGLint value = p[1];
        if((p[0] == EGL_RENDERABLE_TYPE || p[0] == EGL_CONFORMANT) && (value & EGL_OPENGL_BIT))
            value = (value & ~EGL_OPENGL_BIT) | EGL_OPENGL_ES3_BIT_KHR;
        attributes.push_back(p[0]);
        attributes.push_back(value);
    }
    attributes.push_back(EGL_NONE);
    return pEGL_ChooseConfig ? pEGL_ChooseConfig(dpy, attributes.data(), configs, config_size, num_config) : EGL_FALSE;
}
GLINAPI EGLContext EXPORT eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list) {
    if(!pEGL_CreateContext) return EGL_NO_CONTEXT;
    pEGL_BindAPI(EGL_OPENGL_ES_API);
    EGLContext context = pEGL_CreateContext(dpy, config, share_list, egl_attrib);
    if(context != EGL_NO_CONTEXT)
    {
        std::lock_guard<std::mutex> lock(contextMutex);
        auto shared = contexts.find(share_list);
        contexts[context] = shared == contexts.end() ? std::make_shared<glin_globals_t>() : std::make_shared<glin_globals_t>(shared->second->objects);
    }
    return context;
}
GLINAPI EGLSurface EXPORT eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list) { return pEGL_CreateWindowSurface(dpy, config, window, attrib_list); }
GLINAPI EGLBoolean EXPORT eglDestroyContext(EGLDisplay dpy, EGLContext ctx) {
    if(!pEGL_DestroyContext) return EGL_FALSE;
    EGLBoolean result = pEGL_DestroyContext(dpy, ctx);
    if(result) { std::lock_guard<std::mutex> lock(contextMutex); contexts.erase(ctx); }
    return result;
}
GLINAPI EGLBoolean EXPORT eglDestroySurface(EGLDisplay dpy, EGLSurface surface) { return pEGL_DestroySurface(dpy, surface); }
GLINAPI EGLContext EXPORT eglGetCurrentContext(void) { return pEGL_GetCurrentContext(); }
GLINAPI EGLSurface EXPORT eglGetCurrentSurface(EGLint readdraw) { return pEGL_GetCurrentSurface(readdraw); }
GLINAPI EGLBoolean EXPORT eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value)
{
    EGLint tmp_val = 0;
    if(!value || !pEGL_GetConfigAttrib) return EGL_FALSE;
    EGLBoolean ret = pEGL_GetConfigAttrib(dpy, config, attribute, &tmp_val);
    if(attribute == EGL_RENDERABLE_TYPE) tmp_val |= EGL_OPENGL_BIT;
    if(ret) *value = tmp_val;
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
GLINAPI EGLBoolean EXPORT eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx) {
    if(!pEGL_MakeCurrent) return EGL_FALSE;
    EGLBoolean result = pEGL_MakeCurrent(dpy, draw, read, ctx);
    if(result)
    {
        std::lock_guard<std::mutex> lock(contextMutex);
        if(ctx == EGL_NO_CONTEXT) currentState.reset();
        else
        {
            auto& state = contexts[ctx];
            if(!state) state = std::make_shared<glin_globals_t>();
            currentState = state;
        }
        globals = currentState ? currentState.get() : &globalsLocal;
    }
    return result;
}
GLINAPI EGLBoolean EXPORT eglReleaseThread(void)
{
    if(!pEGL_ReleaseThread || !pEGL_ReleaseThread()) return EGL_FALSE;
    currentState.reset();
    globals = &globalsLocal;
    return EGL_TRUE;
}
GLINAPI EGLBoolean EXPORT eglSwapBuffers(EGLDisplay dpy, EGLSurface draw) { return pEGL_SwapBuffers(dpy, draw); }
GLINAPI EGLBoolean EXPORT eglSwapInterval(EGLDisplay dpy, EGLint interval) { return pEGL_SwapInterval(dpy, interval); }
GLINAPI EGLBoolean EXPORT eglTerminate(EGLDisplay dpy) { return pEGL_Terminate(dpy); }
GLINAPI EGLenum EXPORT eglQueryAPI() { return EGL_OPENGL_API; }
GLINAPI EXPORT const char* eglQueryString(EGLDisplay dpy, EGLint name) { return pEGL_QueryString(dpy, name); }
GLINAPI EGLDisplay EXPORT eglGetCurrentDisplay() { return pEGL_GetCurrentDisplay(); }

GLINAPI EXPORT void(* eglGetProcAddress(const char *name))(void)
{
    if(!name) return nullptr;
    EGL_MAP(eglGetPlatformDisplay);
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
    EGL_MAP(eglQueryString);
    EGL_MAP(eglGetCurrentDisplay);
    
    if(strncmp(name, "egl", 3) == 0) return (void(*)())dlsym(eglLib, name);
    return (void(*)())GLIN_GetProcAddress(name);
}
