# VidyoWorks NDK Android Integration
Solution to integrate VidyoWorks platform into your android app using NDK layer. Adding more control via IN-OUT events & REQUEST.

### Documentation & SDK:
https://support.vidyocloud.com/hc/en-us/articles/115003651834-VidyoWorks-API-Downloads-

Corresponding 32 & 64 bit architecture binaries.

> Note (64bit support): Pay attention to **jni/Application.mk**. Now **APP_ABI** is defined as: *APP_ABI := armeabi-v7a arm64-v8a* in order to build both architectures. Obiviously, **jni/lib/Android.mk** LOCAL_SRC_FILES param should refer target architecture ABI during build: *LOCAL_SRC_FILES := $(TARGET_ARCH_ABI)/libVidyoClientApp.so*.
**TARGET_ARCH_ABI** - would be replaced with 'armeabi-v7a' or 'arm64-v8a' depends on current build architecture.

### NDK Layer:
- ndkVidyoSample.c

### Corresponding JNI Java Interface:
- JniBridge.java