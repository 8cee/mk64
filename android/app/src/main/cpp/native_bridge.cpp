#include <jni.h>
#include <android/log.h>
#include <atomic>
#include <mutex>
#include <string>

#define MK64_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "MK64", __VA_ARGS__)
#define MK64_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "MK64", __VA_ARGS__)

namespace {
enum class RuntimeStage : int {
    Bootstrap = 1,
    RomSelected = 2,
    PlatformReady = 3,
    GameReady = 4,
    Running = 5,
    Failed = -1,
};

std::atomic<RuntimeStage> g_stage{RuntimeStage::Bootstrap};
std::mutex g_runtimeMutex;
std::string g_romPath;

// Host-side replacement boundary for libultra services.
// MK64 game code will enter through a cooperative Android loop instead of
// starting the original N64 idle/video/audio OS threads.
bool platform_init() {
    MK64_LOGI("platform_init: Android ARM64 host services");
    g_stage = RuntimeStage::PlatformReady;
    return true;
}
}

extern "C" JNIEXPORT jint JNICALL
Java_com_eightcee_mk64_MainActivity_nativeRuntimeVersion(JNIEnv*, jobject) {
    MK64_LOGI("native runtime initialized; stage=%d", static_cast<int>(g_stage.load()));
    return 2;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_eightcee_mk64_MainActivity_nativeRuntimeStage(JNIEnv*, jobject) {
    return static_cast<int>(g_stage.load());
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_eightcee_mk64_MainActivity_nativeSetRomPath(JNIEnv* env, jobject, jstring path) {
    if (path == nullptr) {
        MK64_LOGE("nativeSetRomPath: null path");
        return JNI_FALSE;
    }
    const char* utf = env->GetStringUTFChars(path, nullptr);
    if (!utf) return JNI_FALSE;
    std::lock_guard<std::mutex> lock(g_runtimeMutex);
    g_romPath.assign(utf);
    env->ReleaseStringUTFChars(path, utf);
    g_stage = RuntimeStage::RomSelected;
    MK64_LOGI("ROM selected (path length=%zu)", g_romPath.size());
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_eightcee_mk64_MainActivity_nativeInitPlatform(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(g_runtimeMutex);
    if (!platform_init()) {
        g_stage = RuntimeStage::Failed;
        return JNI_FALSE;
    }
    return JNI_TRUE;
}
