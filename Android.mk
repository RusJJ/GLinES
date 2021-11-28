LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := gl4es

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
	src/egl.cpp \
	src/main.cpp \
	src/math.cpp \
	src/wrapped.cpp \
	src/gl/buffer.cpp \
	src/gl/matrix.cpp \
	src/gl/object.cpp \
	src/gl/queries.cpp \
	src/gl/render.cpp \
	src/gl/shader.cpp \
	src/gl/texture.cpp \
	thirdparty/DXTn.c

LOCAL_CFLAGS += -DDEBUG
LOCAL_CFLAGS += -O2 #-ftree-vectorize -mthumb -mfpu=neon -pipe #-mfloat-abi=softfp
LOCAL_LDLIBS := -llog

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
	LOCAL_LDLIBS += $(LOCAL_PATH)/libGLESv3_ARM32.so
else
	ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
		LOCAL_LDLIBS += $(LOCAL_PATH)/libGLESv3_ARM64.so
	endif
endif

include $(BUILD_SHARED_LIBRARY)