LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := gl4es

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
	src/main.cpp \
	src/math.cpp \
	src/wrapped.cpp \
	src/gl/buffer.cpp \
	src/gl/object.cpp \
	src/gl/render.cpp \
	src/gl/shader_conv.cpp \
	src/gl/matrix.cpp

LOCAL_CFLAGS += -DDEBUG
LOCAL_CFLAGS += -O2 -ftree-vectorize -mthumb -mfpu=neon -mcpu=cortex-a9 -mfloat-abi=softfp -pipe

LOCAL_LDLIBS := -llog $(LOCAL_PATH)/libGLESv3_d.so

include $(BUILD_SHARED_LIBRARY)