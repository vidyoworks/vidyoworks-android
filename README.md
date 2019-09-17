# VidyoWorks NDK Android Integration
VidyoWorks platform Android sample demonstrates VidyoClient SDK implementation via in-out events pre-built on NDK layer.

## Documentation & SDK:
https://support.vidyocloud.com/hc/en-us/articles/115003651834-VidyoWorks-API-Downloads-

## VidyoClient SDK integration & notes

#### Notes
- *com.vidyo.LmiDeviceManager*/... - VidyoClient related package that would be referenced from native side at runtime.
Please do not change the package/files names and keep it as it is: **com.vidyo.LmiDeviceManager/**
You can find it under VidyoClient SDK unpackaged folder.

- */src/main/jni* - NDK pre-built layer related path. Contains NDK configuration files, 'ndkVidyoSample.c' JNI layer implementation, debug header and that's the place where VidyoClient SDK should be located.

- *jniLibs* - output folder for generated binaries as a result of NDK build command (build.sh)

- *ndkVidyoSample.c* - NDK layer file written on C-lang. Responsible for accepting and transmitting in/out events and requests between app layer and VidyoClient SDK.

- *JniBridge.java* - Java related file for JNI linking.

#### Integration

1. Download latest Android VidyoClient SDK at: https://support.vidyocloud.com/hc/en-us/articles/115003651834-VidyoWorks-API-Downloads-
2. Unzip the package and observe "include" & "lib" folders inside.
3. Copy "include" & "lib" folder into ~/src/main/jni/lib/ folder.
> Note: During the NDK pre-build binaries are getting referenced with ~/jni/lib/Android.mk file configurations:
LOCAL_SRC_FILES := lib/$(TARGET_ARCH_ABI)/libVidyoClientApp.so and 'TARGET_ARCH_ABI' replaced with running ABI.
4. Open terminal and run command: ./build.sh
5. Observe both 32 & 64 bit architectures under *~/jni/jniLibs/* created.


Corresponding 32 & 64-bit architecture binaries.

> Note (64bit support): Pay attention to **jni/Application.mk**. Now **APP_ABI** is defined as: *APP_ABI := armeabi-v7a arm64-v8a* in order to build both architectures. Obiviously, **jni/lib/Android.mk** LOCAL_SRC_FILES param should refer target architecture ABI during build: *LOCAL_SRC_FILES := lib/$(TARGET_ARCH_ABI)/libVidyoClientApp.so*.
**TARGET_ARCH_ABI** - would be replaced with 'armeabi-v7a' or 'arm64-v8a' depends on current build architecture.
