LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := gl4es

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include $(LOCAL_PATH)

LOCAL_SRC_FILES := \
	src/egl.cpp \
	src/main.cpp \
	src/glhelper.cpp \
	src/math.cpp \
	src/wrapped.cpp \
	src/gl/buffer.cpp \
	src/gl/matrix.cpp \
	src/gl/object.cpp \
	src/gl/queries.cpp \
	src/gl/render.cpp \
	src/gl/draw.cpp \
	src/gl/shader.cpp \
	src/gl/texture.cpp \
	src/gl/uniform.cpp \
	thirdparty/DXTn.c

LOCAL_CPPFLAGS := -std=c++17 -Wall -Wextra
LOCAL_LDLIBS := -llog -ldl -lGLESv3
include $(BUILD_SHARED_LIBRARY)
