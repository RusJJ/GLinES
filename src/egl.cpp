#include "GLES.h"
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>
#include <memory>
#include <mutex>
#include <dlfcn.h>
#include <limits>
#include <sstream>

GLINAPI EXPORT void* GLIN_GetProcAddress(const char* name);
extern thread_local glin_globals_t globalsLocal;

static void* EGLLibrary()
{
    static void* library = []() {
        void* handle = dlopen("libEGL.so", RTLD_NOW | RTLD_LOCAL);
        return ( handle ? handle : dlopen("libEGL.so.1", RTLD_NOW | RTLD_LOCAL) );
    }();
    return library;
}

static void* EGLProc(const char* name)
{
    void* library = EGLLibrary();
    if(!library)
    {
        return NULL;
    }
    if(void* fn = dlsym(library, name))
    {
        return fn;
    }
    auto getProc = (decltype(&eglGetProcAddress))dlsym(library, "eglGetProcAddress");
    return ( getProc ? (void*)getProc(name) : NULL );
}

static thread_local EGLint eglError = EGL_SUCCESS;
static thread_local EGLenum currentAPI = EGL_OPENGL_ES_API;

static void EGLError(EGLint error)
{
    auto getError = (decltype(&eglGetError))EGLProc("eglGetError");
    if(getError)
    {
        getError();
    }
    eglError = error;
}

template<class F, class R, class... A> static R EGLCall(F fn, R failure, A... args)
{
    if(!fn)
    {
        EGLError(EGL_NOT_INITIALIZED);
        return failure;
    }
    R result = fn(args...);
    auto getError = (decltype(&eglGetError))EGLProc("eglGetError");
    EGLint error = ( getError ? getError() : EGL_SUCCESS );
    if(error != EGL_SUCCESS)
    {
        eglError = error;
    }
    return result;
}

static bool EGLAttributes(const EGLAttrib* input, std::vector<EGLint>& output)
{
    if(input)
    {
        for(const EGLAttrib* p = input; *p != EGL_NONE; p += 2)
        {
            if(p[0] < std::numeric_limits<EGLint>::min() || p[0] > std::numeric_limits<EGLint>::max() ||
               p[1] < std::numeric_limits<EGLint>::min() || p[1] > std::numeric_limits<EGLint>::max())
            {
                EGLError(EGL_BAD_ATTRIBUTE);
                return false;
            }
            output.push_back((EGLint)p[0]);
            output.push_back((EGLint)p[1]);
        }
    }
    output.push_back(EGL_NONE);
    return true;
}

struct egl_context_t
{
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLenum api = EGL_OPENGL_ES_API;
    bool destroyed = false;
    size_t bindings = 0;
    std::shared_ptr<glin_globals_t> state;
};

static std::mutex contextMutex;
static std::unordered_map<EGLContext, std::shared_ptr<egl_context_t>> contexts;
static std::unordered_map<EGLDisplay, std::string> clientAPIs;
static thread_local std::shared_ptr<egl_context_t> currentContext;

static void ReleaseContext()
{
    if(currentContext && currentContext->bindings)
    {
        --currentContext->bindings;
    }
    for(auto it = contexts.begin(); it != contexts.end();)
    {
        if(it->second->destroyed && !it->second->bindings)
        {
            it = contexts.erase(it);
        }
        else
        {
            ++it;
        }
    }
    currentContext.reset();
    globals = &globalsLocal;
}

struct egl_thread_t
{
    ~egl_thread_t()
    {
        auto release = (decltype(&eglReleaseThread))EGLProc("eglReleaseThread");
        if(release)
        {
            release();
        }
        std::lock_guard<std::mutex> lock(contextMutex);
        ReleaseContext();
    }
};
static thread_local egl_thread_t eglThread;

GLINAPI EGLBoolean EXPORT eglCopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target)
{
    static auto fn = (decltype(&eglCopyBuffers))EGLProc("eglCopyBuffers");
    return EGLCall(fn, (EGLBoolean)0, dpy, surface, target);
}

GLINAPI EGLSurface EXPORT eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list)
{
    static auto fn = (decltype(&eglCreatePbufferSurface))EGLProc("eglCreatePbufferSurface");
    return EGLCall(fn, (EGLSurface)NULL, dpy, config, attrib_list);
}

GLINAPI EGLSurface EXPORT eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list)
{
    static auto fn = (decltype(&eglCreatePixmapSurface))EGLProc("eglCreatePixmapSurface");
    return EGLCall(fn, (EGLSurface)NULL, dpy, config, pixmap, attrib_list);
}

GLINAPI EGLSurface EXPORT eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list)
{
    static auto fn = (decltype(&eglCreateWindowSurface))EGLProc("eglCreateWindowSurface");
    return EGLCall(fn, (EGLSurface)NULL, dpy, config, win, attrib_list);
}

GLINAPI EGLBoolean EXPORT eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
    static auto fn = (decltype(&eglDestroySurface))EGLProc("eglDestroySurface");
    return EGLCall(fn, (EGLBoolean)0, dpy, surface);
}

GLINAPI EGLBoolean EXPORT eglGetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config)
{
    static auto fn = (decltype(&eglGetConfigs))EGLProc("eglGetConfigs");
    return EGLCall(fn, (EGLBoolean)0, dpy, configs, config_size, num_config);
}

GLINAPI EGLDisplay EXPORT eglGetCurrentDisplay(void)
{
    static auto fn = (decltype(&eglGetCurrentDisplay))EGLProc("eglGetCurrentDisplay");
    EGLDisplay result = EGLCall(fn, (EGLDisplay)NULL);
    return ( (currentContext && currentContext->api != currentAPI) ? EGL_NO_DISPLAY : result );
}

GLINAPI EGLSurface EXPORT eglGetCurrentSurface(EGLint readdraw)
{
    static auto fn = (decltype(&eglGetCurrentSurface))EGLProc("eglGetCurrentSurface");
    EGLSurface result = EGLCall(fn, (EGLSurface)NULL, readdraw);
    return ( (currentContext && currentContext->api != currentAPI) ? EGL_NO_SURFACE : result );
}

GLINAPI EGLDisplay EXPORT eglGetDisplay(EGLNativeDisplayType display_id)
{
    static auto fn = (decltype(&eglGetDisplay))EGLProc("eglGetDisplay");
    return EGLCall(fn, (EGLDisplay)NULL, display_id);
}

GLINAPI EGLBoolean EXPORT eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
    static auto fn = (decltype(&eglInitialize))EGLProc("eglInitialize");
    return EGLCall(fn, (EGLBoolean)0, dpy, major, minor);
}

GLINAPI EGLBoolean EXPORT eglQuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value)
{
    static auto fn = (decltype(&eglQuerySurface))EGLProc("eglQuerySurface");
    return EGLCall(fn, (EGLBoolean)0, dpy, surface, attribute, value);
}

GLINAPI EGLBoolean EXPORT eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
    static auto fn = (decltype(&eglSwapBuffers))EGLProc("eglSwapBuffers");
    return EGLCall(fn, (EGLBoolean)0, dpy, surface);
}

GLINAPI EGLBoolean EXPORT eglWaitGL(void)
{
    static auto fn = (decltype(&eglWaitGL))EGLProc("eglWaitGL");
    return EGLCall(fn, (EGLBoolean)0);
}

GLINAPI EGLBoolean EXPORT eglWaitNative(EGLint engine)
{
    static auto fn = (decltype(&eglWaitNative))EGLProc("eglWaitNative");
    return EGLCall(fn, (EGLBoolean)0, engine);
}

GLINAPI EGLBoolean EXPORT eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    static auto fn = (decltype(&eglBindTexImage))EGLProc("eglBindTexImage");
    return EGLCall(fn, (EGLBoolean)0, dpy, surface, buffer);
}

GLINAPI EGLBoolean EXPORT eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
    static auto fn = (decltype(&eglReleaseTexImage))EGLProc("eglReleaseTexImage");
    return EGLCall(fn, (EGLBoolean)0, dpy, surface, buffer);
}

GLINAPI EGLBoolean EXPORT eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value)
{
    static auto fn = (decltype(&eglSurfaceAttrib))EGLProc("eglSurfaceAttrib");
    return EGLCall(fn, (EGLBoolean)0, dpy, surface, attribute, value);
}

GLINAPI EGLBoolean EXPORT eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
    static auto fn = (decltype(&eglSwapInterval))EGLProc("eglSwapInterval");
    return EGLCall(fn, (EGLBoolean)0, dpy, interval);
}

GLINAPI EGLSurface EXPORT eglCreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list)
{
    static auto fn = (decltype(&eglCreatePbufferFromClientBuffer))EGLProc("eglCreatePbufferFromClientBuffer");
    return EGLCall(fn, (EGLSurface)NULL, dpy, buftype, buffer, config, attrib_list);
}

GLINAPI EGLBoolean EXPORT eglWaitClient(void)
{
    static auto fn = (decltype(&eglWaitClient))EGLProc("eglWaitClient");
    return EGLCall(fn, (EGLBoolean)0);
}

GLINAPI EGLContext EXPORT eglGetCurrentContext(void)
{
    static auto fn = (decltype(&eglGetCurrentContext))EGLProc("eglGetCurrentContext");
    EGLContext result = EGLCall(fn, (EGLContext)NULL);
    return ( (currentContext && currentContext->api != currentAPI) ? EGL_NO_CONTEXT : result );
}

GLINAPI EGLSync EXPORT eglCreateSync(EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list)
{
    static auto fn = (decltype(&eglCreateSync))EGLProc("eglCreateSync");
    if(!fn)
    {
        std::vector<EGLint> attributes;
        if(!EGLAttributes(attrib_list, attributes))
        {
            return (EGLSync)NULL;
        }
        static auto extension = (decltype(&eglCreateSyncKHR))EGLProc("eglCreateSyncKHR");
        return (EGLSync)EGLCall(extension, (EGLSync)NULL, dpy, type, attributes.data());
    }
    return EGLCall(fn, (EGLSync)NULL, dpy, type, attrib_list);
}

GLINAPI EGLBoolean EXPORT eglDestroySync(EGLDisplay dpy, EGLSync sync)
{
    static auto fn = (decltype(&eglDestroySync))EGLProc("eglDestroySync");
    if(!fn)
    {
        static auto extension = (decltype(&eglDestroySyncKHR))EGLProc("eglDestroySyncKHR");
        return EGLCall(extension, (EGLBoolean)0, dpy, sync);
    }
    return EGLCall(fn, (EGLBoolean)0, dpy, sync);
}

GLINAPI EGLint EXPORT eglClientWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout)
{
    static auto fn = (decltype(&eglClientWaitSync))EGLProc("eglClientWaitSync");
    if(!fn)
    {
        static auto extension = (decltype(&eglClientWaitSyncKHR))EGLProc("eglClientWaitSyncKHR");
        return EGLCall(extension, (EGLint)0, dpy, sync, flags, timeout);
    }
    return EGLCall(fn, (EGLint)0, dpy, sync, flags, timeout);
}

GLINAPI EGLBoolean EXPORT eglGetSyncAttrib(EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib *value)
{
    if(!value)
    {
        EGLError(EGL_BAD_PARAMETER);
        return EGL_FALSE;
    }
    static auto fn = (decltype(&eglGetSyncAttrib))EGLProc("eglGetSyncAttrib");
    if(!fn)
    {
        static auto extension = (decltype(&eglGetSyncAttribKHR))EGLProc("eglGetSyncAttribKHR");
        EGLint native = 0;
        EGLBoolean result = EGLCall(extension, EGL_FALSE, dpy, sync, attribute, &native);
        if(result)
        {
            *value = native;
        }
        return result;
    }
    return EGLCall(fn, (EGLBoolean)0, dpy, sync, attribute, value);
}

GLINAPI EGLImage EXPORT eglCreateImage(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLAttrib *attrib_list)
{
    static auto fn = (decltype(&eglCreateImage))EGLProc("eglCreateImage");
    if(!fn)
    {
        std::vector<EGLint> attributes;
        if(!EGLAttributes(attrib_list, attributes))
        {
            return (EGLImage)NULL;
        }
        static auto extension = (decltype(&eglCreateImageKHR))EGLProc("eglCreateImageKHR");
        return (EGLImage)EGLCall(extension, (EGLImage)NULL, dpy, ctx, target, buffer, attributes.data());
    }
    return EGLCall(fn, (EGLImage)NULL, dpy, ctx, target, buffer, attrib_list);
}

GLINAPI EGLBoolean EXPORT eglDestroyImage(EGLDisplay dpy, EGLImage image)
{
    static auto fn = (decltype(&eglDestroyImage))EGLProc("eglDestroyImage");
    if(!fn)
    {
        static auto extension = (decltype(&eglDestroyImageKHR))EGLProc("eglDestroyImageKHR");
        return EGLCall(extension, (EGLBoolean)0, dpy, image);
    }
    return EGLCall(fn, (EGLBoolean)0, dpy, image);
}

GLINAPI EGLDisplay EXPORT eglGetPlatformDisplay(EGLenum platform, void *native_display, const EGLAttrib *attrib_list)
{
    static auto fn = (decltype(&eglGetPlatformDisplay))EGLProc("eglGetPlatformDisplay");
    if(!fn)
    {
        std::vector<EGLint> attributes;
        if(!EGLAttributes(attrib_list, attributes))
        {
            return (EGLDisplay)NULL;
        }
        static auto extension = (decltype(&eglGetPlatformDisplayEXT))EGLProc("eglGetPlatformDisplayEXT");
        return (EGLDisplay)EGLCall(extension, (EGLDisplay)NULL, platform, native_display, attributes.data());
    }
    return EGLCall(fn, (EGLDisplay)NULL, platform, native_display, attrib_list);
}

GLINAPI EGLSurface EXPORT eglCreatePlatformWindowSurface(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLAttrib *attrib_list)
{
    static auto fn = (decltype(&eglCreatePlatformWindowSurface))EGLProc("eglCreatePlatformWindowSurface");
    if(!fn)
    {
        std::vector<EGLint> attributes;
        if(!EGLAttributes(attrib_list, attributes))
        {
            return (EGLSurface)NULL;
        }
        static auto extension = (decltype(&eglCreatePlatformWindowSurfaceEXT))EGLProc("eglCreatePlatformWindowSurfaceEXT");
        return (EGLSurface)EGLCall(extension, (EGLSurface)NULL, dpy, config, native_window, attributes.data());
    }
    return EGLCall(fn, (EGLSurface)NULL, dpy, config, native_window, attrib_list);
}

GLINAPI EGLSurface EXPORT eglCreatePlatformPixmapSurface(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list)
{
    static auto fn = (decltype(&eglCreatePlatformPixmapSurface))EGLProc("eglCreatePlatformPixmapSurface");
    if(!fn)
    {
        std::vector<EGLint> attributes;
        if(!EGLAttributes(attrib_list, attributes))
        {
            return (EGLSurface)NULL;
        }
        static auto extension = (decltype(&eglCreatePlatformPixmapSurfaceEXT))EGLProc("eglCreatePlatformPixmapSurfaceEXT");
        return (EGLSurface)EGLCall(extension, (EGLSurface)NULL, dpy, config, native_pixmap, attributes.data());
    }
    return EGLCall(fn, (EGLSurface)NULL, dpy, config, native_pixmap, attrib_list);
}

GLINAPI EGLBoolean EXPORT eglWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags)
{
    static auto fn = (decltype(&eglWaitSync))EGLProc("eglWaitSync");
    if(!fn)
    {
        static auto extension = (decltype(&eglWaitSyncKHR))EGLProc("eglWaitSyncKHR");
        return EGLCall(extension, (EGLBoolean)0, dpy, sync, flags);
    }
    return EGLCall(fn, (EGLBoolean)0, dpy, sync, flags);
}


GLINAPI EGLint EXPORT eglGetError(void)
{
    auto fn = (decltype(&eglGetError))EGLProc("eglGetError");
    EGLint native = ( fn ? fn() : EGL_SUCCESS );
    EGLint result = ( (native != EGL_SUCCESS) ? native : eglError );
    eglError = EGL_SUCCESS;
    return result;
}

GLINAPI EGLBoolean EXPORT eglBindAPI(EGLenum api)
{
    if(api != EGL_OPENGL_API && api != EGL_OPENGL_ES_API && api != EGL_OPENVG_API)
    {
        EGLError(EGL_BAD_PARAMETER);
        return EGL_FALSE;
    }
    static auto fn = (decltype(&eglBindAPI))EGLProc("eglBindAPI");
    EGLBoolean result = EGLCall(fn, EGL_FALSE, ( (api == EGL_OPENGL_API) ? EGL_OPENGL_ES_API : api ));
    if(result)
    {
        currentAPI = api;
    }
    return result;
}

GLINAPI EGLenum EXPORT eglQueryAPI(void) { return currentAPI; }

GLINAPI EGLBoolean EXPORT eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint* value)
{
    if(!value)
    {
        EGLError(EGL_BAD_PARAMETER);
        return EGL_FALSE;
    }
    static auto fn = (decltype(&eglGetConfigAttrib))EGLProc("eglGetConfigAttrib");
    EGLBoolean result = EGLCall(fn, EGL_FALSE, dpy, config, attribute, value);
    if(result && attribute == EGL_RENDERABLE_TYPE)
    {
        *value &= ~EGL_OPENGL_BIT;
        if(*value & EGL_OPENGL_ES3_BIT)
        {
            *value |= EGL_OPENGL_BIT;
        }
    }
    if(result && attribute == EGL_CONFORMANT)
    {
        *value &= ~EGL_OPENGL_BIT;
    }
    return result;
}

GLINAPI EGLBoolean EXPORT eglChooseConfig(EGLDisplay dpy, const EGLint* attrib_list, EGLConfig* configs, EGLint config_size, EGLint* num_config)
{
    std::vector<EGLint> attributes;
    bool desktopConformant = false;
    EGLint configID = EGL_DONT_CARE;
    if(attrib_list)
    {
        for(const EGLint* p = attrib_list; *p != EGL_NONE; p += 2)
        {
            EGLint value = p[1];
            if(p[0] == EGL_CONFIG_ID)
            {
                configID = value;
            }
            if(p[0] == EGL_CONFORMANT)
            {
                desktopConformant = (value != EGL_DONT_CARE && (value & EGL_OPENGL_BIT));
            }
            if(value != EGL_DONT_CARE && p[0] == EGL_RENDERABLE_TYPE && (value & EGL_OPENGL_BIT))
            {
                value = (value & ~EGL_OPENGL_BIT) | EGL_OPENGL_ES3_BIT;
            }
            if(value != EGL_DONT_CARE && p[0] == EGL_CONFORMANT && (value & EGL_OPENGL_BIT))
            {
                value &= ~EGL_OPENGL_BIT;
            }
            attributes.push_back(p[0]);
            attributes.push_back(value);
        }
    }
    attributes.push_back(EGL_NONE);
    static auto fn = (decltype(&eglChooseConfig))EGLProc("eglChooseConfig");
    EGLBoolean result = EGLCall(fn, EGL_FALSE, dpy, attributes.data(), configs, config_size, num_config);
    if(result && desktopConformant && configID == EGL_DONT_CARE)
    {
        *num_config = 0;
    }
    return result;
}

GLINAPI EGLContext EXPORT eglCreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint* attrib_list)
{
    std::lock_guard<std::mutex> lock(contextMutex);
    auto shared = contexts.find(share_context);
    if(shared != contexts.end() && shared->second->destroyed)
    {
        EGLError(EGL_BAD_CONTEXT);
        return EGL_NO_CONTEXT;
    }
    if(shared != contexts.end() && (shared->second->api != currentAPI || shared->second->display != dpy))
    {
        EGLError(EGL_BAD_MATCH);
        return EGL_NO_CONTEXT;
    }
    EGLint profile = EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT, flags = 0;
    std::vector<EGLint> attributes;
    if(currentAPI == EGL_OPENGL_API)
    {
        EGLint major = 1, minor = 0;
        bool profileSet = false;
        if(attrib_list)
        {
            for(const EGLint* p = attrib_list; *p != EGL_NONE; p += 2)
            {
                switch(p[0])
                {
                    case EGL_CONTEXT_MAJOR_VERSION: major = p[1]; break;
                    case EGL_CONTEXT_MINOR_VERSION: minor = p[1]; break;
                    case EGL_CONTEXT_OPENGL_PROFILE_MASK: profile = p[1]; profileSet = true; break;
                    case EGL_CONTEXT_FLAGS_KHR:
                        if(p[1] & ~(EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR | EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR | EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR))
                        {
                            EGLError(EGL_BAD_ATTRIBUTE);
                            return EGL_NO_CONTEXT;
                        }
                        flags = p[1]; break;
                    case EGL_CONTEXT_OPENGL_DEBUG:
                    case EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE:
                    case EGL_CONTEXT_OPENGL_ROBUST_ACCESS:
                    {
                        if(p[1] != EGL_TRUE && p[1] != EGL_FALSE)
                        {
                            EGLError(EGL_BAD_ATTRIBUTE);
                            return EGL_NO_CONTEXT;
                        }
                        EGLint bit = EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR;
                        if(p[0] == EGL_CONTEXT_OPENGL_DEBUG)
                        {
                            bit = EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
                        }
                        else if(p[0] == EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE)
                        {
                            bit = EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR;
                        }
                        if(p[1])
                        {
                            flags |= bit;
                        }
                        else
                        {
                            flags &= ~bit;
                        }
                        break;
                    }
                    case EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY:
                        if(p[1] != EGL_NO_RESET_NOTIFICATION && p[1] != EGL_LOSE_CONTEXT_ON_RESET)
                        {
                            EGLError(EGL_BAD_ATTRIBUTE);
                            return EGL_NO_CONTEXT;
                        }
                        if(p[1] != EGL_NO_RESET_NOTIFICATION)
                        {
                            EGLError(EGL_BAD_MATCH);
                            return EGL_NO_CONTEXT;
                        }
                        break;
                    default:
                        EGLError(EGL_BAD_ATTRIBUTE);
                        return EGL_NO_CONTEXT;
                }
            }
        }
        if(major < 1 || minor < 0)
        {
            EGLError(EGL_BAD_ATTRIBUTE);
            return EGL_NO_CONTEXT;
        }
        if(major > 3 || (major == 3 && minor > 3) || (major == 2 && minor > 1) || (major == 1 && minor > 5))
        {
            EGLError(EGL_BAD_MATCH);
            return EGL_NO_CONTEXT;
        }
        if(major == 3 && minor >= 2)
        {
            if(!profileSet)
            {
                profile = EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT;
            }
            if(profile != EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT && profile != EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT)
            {
                EGLError(EGL_BAD_ATTRIBUTE);
                return EGL_NO_CONTEXT;
            }
        }
        else
        {
            profile = ( (major == 3 && minor == 1) ? EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT : EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT );
        }
        if(flags & (EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR | EGL_CONTEXT_OPENGL_ROBUST_ACCESS_BIT_KHR))
        {
            EGLError(EGL_BAD_MATCH);
            return EGL_NO_CONTEXT;
        }
        attributes = {EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 2};
        if(flags)
        {
            attributes.push_back(EGL_CONTEXT_FLAGS_KHR);
            attributes.push_back(flags);
        }
        attributes.push_back(EGL_NONE);
        attrib_list = attributes.data();
    }
    static auto fn = (decltype(&eglCreateContext))EGLProc("eglCreateContext");
    EGLContext context = EGLCall(fn, EGL_NO_CONTEXT, dpy, config, share_context, attrib_list);
    if(context == EGL_NO_CONTEXT)
    {
        return context;
    }
    auto record = std::make_shared<egl_context_t>();
    record->display = dpy;
    record->api = currentAPI;
    record->state = ( (shared == contexts.end()) ? std::make_shared<glin_globals_t>() : std::make_shared<glin_globals_t>(shared->second->state->objects) );
    record->state->contextProfile = profile;
    record->state->contextFlags = ( (flags & EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR) ? 2 : 0 );
    contexts[context] = record;
    return context;
}

GLINAPI EGLBoolean EXPORT eglQueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint* value)
{
    if(!value)
    {
        EGLError(EGL_BAD_PARAMETER);
        return EGL_FALSE;
    }
    static auto fn = (decltype(&eglQueryContext))EGLProc("eglQueryContext");
    EGLint native = 0;
    EGLBoolean result = EGLCall(fn, EGL_FALSE, dpy, ctx, EGL_CONTEXT_CLIENT_TYPE, &native);
    if(!result)
    {
        return result;
    }
    std::lock_guard<std::mutex> lock(contextMutex);
    auto it = contexts.find(ctx);
    if(it != contexts.end() && it->second->api == EGL_OPENGL_API)
    {
        if(attribute == EGL_CONTEXT_CLIENT_TYPE)
        {
            *value = EGL_OPENGL_API;
            return EGL_TRUE;
        }
        if(attribute == EGL_CONTEXT_CLIENT_VERSION)
        {
            *value = 3;
            return EGL_TRUE;
        }
    }
    return EGLCall(fn, EGL_FALSE, dpy, ctx, attribute, value);
}

GLINAPI EGLBoolean EXPORT eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
    std::lock_guard<std::mutex> lock(contextMutex);
    static auto fn = (decltype(&eglDestroyContext))EGLProc("eglDestroyContext");
    EGLBoolean result = EGLCall(fn, EGL_FALSE, dpy, ctx);
    if(result)
    {
        auto it = contexts.find(ctx);
        if(it != contexts.end())
        {
            it->second->destroyed = true;
            if(!it->second->bindings)
            {
                contexts.erase(it);
            }
        }
    }
    return result;
}

GLINAPI EGLBoolean EXPORT eglMakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx)
{
    (void)currentContext;
    (void)eglThread;
    {
        std::lock_guard<std::mutex> lock(contextMutex);
        auto selected = contexts.find(ctx);
        if(selected != contexts.end() && selected->second->api != currentAPI)
        {
            EGLError(EGL_BAD_MATCH);
            return EGL_FALSE;
        }
        static auto fn = (decltype(&eglMakeCurrent))EGLProc("eglMakeCurrent");
        EGLBoolean result = EGLCall(fn, EGL_FALSE, dpy, draw, read, ctx);
        if(!result)
        {
            return result;
        }
        auto it = contexts.find(ctx);
        std::shared_ptr<egl_context_t> next = ( (it == contexts.end()) ? std::shared_ptr<egl_context_t>() : it->second );
        if(ctx != EGL_NO_CONTEXT && !next)
        {
            next = std::make_shared<egl_context_t>();
            next->display = dpy;
            next->api = currentAPI;
            next->state = std::make_shared<glin_globals_t>();
            contexts[ctx] = next;
        }
        if(next)
        {
            ++next->bindings;
        }
        ReleaseContext();
        currentContext = next;
        globals = ( next ? next->state.get() : &globalsLocal );
    }
    if(currentContext && currentContext->api == EGL_OPENGL_API)
    {
        GLIN_InitExtensions();
    }
    return EGL_TRUE;
}

GLINAPI EGLBoolean EXPORT eglReleaseThread(void)
{
    static auto fn = (decltype(&eglReleaseThread))EGLProc("eglReleaseThread");
    if(!EGLCall(fn, EGL_FALSE))
    {
        return EGL_FALSE;
    }
    std::lock_guard<std::mutex> lock(contextMutex);
    ReleaseContext();
    currentAPI = EGL_OPENGL_ES_API;
    return EGL_TRUE;
}

GLINAPI EGLBoolean EXPORT eglTerminate(EGLDisplay dpy)
{
    std::lock_guard<std::mutex> lock(contextMutex);
    static auto fn = (decltype(&eglTerminate))EGLProc("eglTerminate");
    EGLBoolean result = EGLCall(fn, EGL_FALSE, dpy);
    if(result)
    {
        for(auto it = contexts.begin(); it != contexts.end();)
        {
            if(it->second->display == dpy)
            {
                it->second->destroyed = true;
            }
            if(it->second->destroyed && !it->second->bindings)
            {
                it = contexts.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    if(result)
    {
        clientAPIs.erase(dpy);
    }
    return result;
}

GLINAPI const char* EXPORT eglQueryString(EGLDisplay dpy, EGLint name)
{
    static auto fn = (decltype(&eglQueryString))EGLProc("eglQueryString");
    const char* result = EGLCall(fn, (const char*)NULL, dpy, name);
    if(result && name == EGL_CLIENT_APIS)
    {
        std::lock_guard<std::mutex> lock(contextMutex);
        auto found = clientAPIs.find(dpy);
        if(found != clientAPIs.end())
        {
            return found->second.c_str();
        }
        std::string apis = result;
        std::istringstream stream(apis);
        std::string api;
        bool desktop = false, es = false;
        while(stream >> api)
        {
            desktop |= api == "OpenGL";
            es |= api == "OpenGL_ES";
        }
        if(es && !desktop)
        {
            apis += " OpenGL";
        }
        return clientAPIs.emplace(dpy, std::move(apis)).first->second.c_str();
    }
    return result;
}

GLINAPI EGLDisplay EXPORT eglGetPlatformDisplayEXT(EGLenum platform, void* native_display, const EGLint* attrib_list)
{
    static auto fn = (decltype(&eglGetPlatformDisplayEXT))EGLProc("eglGetPlatformDisplayEXT");
    return EGLCall(fn, EGL_NO_DISPLAY, platform, native_display, attrib_list);
}

GLINAPI EGLSurface EXPORT eglCreatePlatformWindowSurfaceEXT(EGLDisplay dpy, EGLConfig config, void* native_window, const EGLint* attrib_list)
{
    static auto fn = (decltype(&eglCreatePlatformWindowSurfaceEXT))EGLProc("eglCreatePlatformWindowSurfaceEXT");
    return EGLCall(fn, EGL_NO_SURFACE, dpy, config, native_window, attrib_list);
}

GLINAPI EGLSurface EXPORT eglCreatePlatformPixmapSurfaceEXT(EGLDisplay dpy, EGLConfig config, void* native_pixmap, const EGLint* attrib_list)
{
    static auto fn = (decltype(&eglCreatePlatformPixmapSurfaceEXT))EGLProc("eglCreatePlatformPixmapSurfaceEXT");
    return EGLCall(fn, EGL_NO_SURFACE, dpy, config, native_pixmap, attrib_list);
}

GLINAPI EXPORT void(*eglGetProcAddress(const char* name))(void)
{
    if(!name)
    {
        return NULL;
    }
#define EGL_MAP(fn) if(strcmp(name, #fn) == 0) return (void(*)())fn
    EGL_MAP(eglQueryString);
    EGL_MAP(eglChooseConfig);
    EGL_MAP(eglCopyBuffers);
    EGL_MAP(eglCreateContext);
    EGL_MAP(eglCreatePbufferSurface);
    EGL_MAP(eglCreatePixmapSurface);
    EGL_MAP(eglCreateWindowSurface);
    EGL_MAP(eglDestroyContext);
    EGL_MAP(eglDestroySurface);
    EGL_MAP(eglGetConfigAttrib);
    EGL_MAP(eglGetConfigs);
    EGL_MAP(eglGetCurrentDisplay);
    EGL_MAP(eglGetCurrentSurface);
    EGL_MAP(eglGetDisplay);
    EGL_MAP(eglGetError);
    EGL_MAP(eglGetProcAddress);
    EGL_MAP(eglInitialize);
    EGL_MAP(eglMakeCurrent);
    EGL_MAP(eglQueryContext);
    EGL_MAP(eglQuerySurface);
    EGL_MAP(eglSwapBuffers);
    EGL_MAP(eglTerminate);
    EGL_MAP(eglWaitGL);
    EGL_MAP(eglWaitNative);
    EGL_MAP(eglBindTexImage);
    EGL_MAP(eglReleaseTexImage);
    EGL_MAP(eglSurfaceAttrib);
    EGL_MAP(eglSwapInterval);
    EGL_MAP(eglBindAPI);
    EGL_MAP(eglQueryAPI);
    EGL_MAP(eglCreatePbufferFromClientBuffer);
    EGL_MAP(eglReleaseThread);
    EGL_MAP(eglWaitClient);
    EGL_MAP(eglGetCurrentContext);
    EGL_MAP(eglCreateSync);
    EGL_MAP(eglDestroySync);
    EGL_MAP(eglClientWaitSync);
    EGL_MAP(eglGetSyncAttrib);
    EGL_MAP(eglCreateImage);
    EGL_MAP(eglDestroyImage);
    EGL_MAP(eglGetPlatformDisplay);
    EGL_MAP(eglCreatePlatformWindowSurface);
    EGL_MAP(eglCreatePlatformPixmapSurface);
    EGL_MAP(eglWaitSync);
    EGL_MAP(eglGetPlatformDisplayEXT);
    EGL_MAP(eglCreatePlatformWindowSurfaceEXT);
    EGL_MAP(eglCreatePlatformPixmapSurfaceEXT);
#undef EGL_MAP
    if(strncmp(name, "egl", 3) == 0)
    {
        return (void(*)())EGLProc(name);
    }
    return (void(*)())GLIN_GetProcAddress(name);
}
