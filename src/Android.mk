LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := smallpot
LOCAL_CPPFLAGS := -fexceptions -std=c++20

SDL_PATH := ../SDL3
SDL_IMAGE_PATH := ../SDL3_image
ICONV_PATH := ../iconv
FFMPEG_PATH := ../ffmpeg

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include $(LOCAL_PATH)/$(SDL_IMAGE_PATH)/include $(LOCAL_PATH)/$(FFMPEG_PATH)/include
LOCAL_SHARED_LIBRARIES := SDL3 SDL3_image avcodec avfilter avformat avutil swresample swscale

LOCAL_SRC_FILES := Engine.cpp PotBase.cpp PotDll.cpp PotMedia.cpp PotPlayer.cpp PotResample.cpp PotStream.cpp PotStreamAudio.cpp PotStreamVideo.cpp filefunc.cpp
LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)
