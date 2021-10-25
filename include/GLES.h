#ifndef GLIN2GLES_H
#define GLIN2GLES_H

    #include "GLinES.h"

    #define GL_GLEXT_PROTOTYPES

    #include <GLES3/gl3.h>
    #include <GLES3/gl31.h>
    #include <GLES3/gl32.h>
    #include <GLES3/gl3platform.h>
    #include <GLES2/gl2.h>
    #include <GLES2/gl2ext.h>
    #include <GLES2/gl2platform.h>

    #include <GLES/gl.h> // GL_MODELVIEW and more

    #include <string.h>
    #include <stdbool.h>

    #include "globals.h"

    #ifdef DEBUG
        #include <android/log.h>
        #define DBG(...) __android_log_print(ANDROID_LOG_INFO, "GLinES", __VA_ARGS__)
        #define ERR(...) __android_log_print(ANDROID_LOG_ERROR, "GLinES", __VA_ARGS__)
    #else
        #define DBG(...)
        #define ERR(...)
    #endif

    #ifdef STATIC_LIB
        #define EXPORT
    #else
        #define EXPORT __attribute__((visibility("default")))
    #endif
    #define EXPORT_ALIAS(__fn_name) __attribute__((alias(#__fn_name)))

    #ifdef __cplusplus
        #define GLINAPI extern "C"
    #else
        #error Sorry, raw C is not supported! Switch to the C++.
        //#define GLINAPI
    #endif

    extern void *(*pSetGetProcAddr)(const char* name);

#endif // GLIN2GLES_H