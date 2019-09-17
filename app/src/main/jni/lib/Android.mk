LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := VidyoClientApp

# Refer specific folder during build process
LOCAL_SRC_FILES := lib/$(TARGET_ARCH_ABI)/libVidyoClientApp.so

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

include $(PREBUILT_SHARED_LIBRARY)