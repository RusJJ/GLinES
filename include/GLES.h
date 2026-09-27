#ifndef GLINES_H
#define GLINES_H


    #include "GLinES_Config.h"
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
    #include <signal.h>

// Macro for 32/64 bits
    #if defined(__aarch64__) || defined(__x86_64__)
        #define SYS64
    #else
        #define SYS32
    #endif

// LogCat messages
    #ifdef __ANDROID__
        #include <android/log.h>
        #define MSG(...) __android_log_print(ANDROID_LOG_INFO, "GLinES", __VA_ARGS__)
        #define ERR(...) __android_log_print(ANDROID_LOG_ERROR, "GLinES", __VA_ARGS__)
    #else
        #include <cstdio>
        #define MSG(...) do { fprintf(stderr, __VA_ARGS__); fputc('\n', stderr); } while(0)
        #define ERR(...) MSG(__VA_ARGS__)
    #endif
    #ifdef DEBUG
        #define DBG(...) MSG(__VA_ARGS__)
    #else
        #define DBG(...) ((void)0)
    #endif

// A visibility of a function
    #ifdef STATIC_LIB
        #define EXPORT
    #else
        #define EXPORT __attribute__((visibility("default")))
    #endif
    #define ALIAS(__fn_name) __attribute__((alias(#__fn_name)))
    #define ALIASWRAP(__fn_name) __attribute__((alias("GLIN_Wrap_" #__fn_name)))

    #ifdef __cplusplus
        #define GLINAPI extern "C"
    #else
        #error Sorry, raw C is not supported! Switch to the C++.
        #define GLINAPI
    #endif

    #define STRINGIFY(a)  __STRINGIFY(a)
    #define __STRINGIFY(a)  #a
    #define WRAP(a) GLIN_Wrap_##a
    #define WRAPCALL(a) do { a; } while(0)

    extern void *(*pSetGetProcAddr)(const char* name);

    typedef double GLdouble;
    #include "desktop_enums.h"
    #include "globals.h"

#endif // GLINES_H
