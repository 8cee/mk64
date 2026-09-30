#include <jni.h>
#include <android/log.h>

extern "C" JNIEXPORT jint JNICALL
Java_com_eightcee_mk64_MainActivity_nativeRuntimeVersion(JNIEnv*, jobject) {
    __android_log_print(ANDROID_LOG_INFO, "MK64", "native runtime initialized");
    return 1;
}
