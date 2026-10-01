#include <jni.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <atomic>
#include <cstdint>
#include <fstream>
#include <array>
#include <chrono>
#include "host_clock.h"
#include "host_input.h"
#include "host_controller_adapter.h"
#include "host_rom.h"
#include "host_pi.h"
#include "game_runtime.h"
#include <mutex>
#include <string>

#define MK64_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "MK64", __VA_ARGS__)
#define MK64_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "MK64", __VA_ARGS__)

extern "C" void mk64_android_gfx_set_size(unsigned width, unsigned height);
extern "C" bool mk64_android_load_assets();

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
std::string g_romIdentity = "none";
Mk64HostClock g_hostClock{};
Mk64RomImage g_romImage;
Mk64HostPi g_hostPi(&g_romImage);

uint64_t monotonic_now_ns() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

// Host-side replacement boundary for libultra services.
// MK64 game code will enter through a cooperative Android loop instead of
// starting the original N64 idle/video/audio OS threads.
bool platform_init() {
    MK64_LOGI("platform_init: Android ARM64 host services");
    mk64_host_clock_reset(&g_hostClock, monotonic_now_ns());
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
    std::string candidate(utf);
    env->ReleaseStringUTFChars(path, utf);

    std::ifstream rom(candidate, std::ios::binary | std::ios::ate);
    if (!rom) {
        MK64_LOGE("ROM validation: unable to open file");
        return JNI_FALSE;
    }
    const auto size = rom.tellg();
    if (size < static_cast<std::streamoff>(8 * 1024 * 1024)) {
        MK64_LOGE("ROM validation: file too small (%lld bytes)", static_cast<long long>(size));
        return JNI_FALSE;
    }
    rom.seekg(0);
    uint8_t header[4] = {};
    rom.read(reinterpret_cast<char*>(header), sizeof(header));
    const bool z64 = header[0] == 0x80 && header[1] == 0x37 && header[2] == 0x12 && header[3] == 0x40;
    const bool v64 = header[0] == 0x37 && header[1] == 0x80 && header[2] == 0x40 && header[3] == 0x12;
    const bool n64 = header[0] == 0x40 && header[1] == 0x12 && header[2] == 0x37 && header[3] == 0x80;
    if (!z64 && !v64 && !n64) {
        MK64_LOGE("ROM validation: unsupported N64 byte order");
        return JNI_FALSE;
    }

    // N64 internal header: game code at 0x3B..0x3E for native big-endian .z64.
    // The Android port currently targets the US asset/config path (NKTE).
    std::array<uint8_t, 0x40> n64Header{};
    rom.clear();
    rom.seekg(0);
    rom.read(reinterpret_cast<char*>(n64Header.data()), n64Header.size());
    auto byteAt = [&](size_t offset) -> uint8_t {
        if (z64) return n64Header[offset];
        if (v64) return n64Header[offset ^ 1];
        return n64Header[(offset & ~3u) + (3u - (offset & 3u))];
    };
    std::string gameCode;
    for (size_t i = 0x3B; i <= 0x3E; ++i) gameCode.push_back(static_cast<char>(byteAt(i)));
    if (gameCode != "NKTE") {
        MK64_LOGE("ROM validation: unsupported game code '%s' (expected NKTE / USA)", gameCode.c_str());
        return JNI_FALSE;
    }

    std::lock_guard<std::mutex> lock(g_runtimeMutex);
    std::string loadError;
    if (!g_romImage.load(candidate, &loadError)) {
        MK64_LOGE("ROM load: %s", loadError.c_str());
        g_stage = RuntimeStage::Failed;
        return JNI_FALSE;
    }
    g_romIdentity = "Mario Kart 64 USA (NKTE)";
    g_romPath = std::move(candidate);
    g_stage = RuntimeStage::RomSelected;
    MK64_LOGI("ROM validated: %lld bytes, %s, order=%s", static_cast<long long>(size), g_romIdentity.c_str(),
              z64 ? "z64" : (v64 ? "v64" : "n64"));

    // ROM import stops here. Asset reconstruction and game initialization are
    // separate JNI stages so the Android shell can persist a crash checkpoint.
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_eightcee_mk64_MainActivity_nativeLoadAssets(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(g_runtimeMutex);
    if (g_romPath.empty() || g_romImage.size() == 0) {
        MK64_LOGE("nativeLoadAssets: no ROM loaded");
        return JNI_FALSE;
    }
    MK64_LOGI("nativeLoadAssets: begin");
    if (!mk64_android_load_assets()) {
        MK64_LOGE("ROM asset reconstruction failed");
        g_stage = RuntimeStage::Failed;
        return JNI_FALSE;
    }
    MK64_LOGI("nativeLoadAssets: complete");
    return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_eightcee_mk64_MainActivity_nativeInitGame(JNIEnv*, jobject) {
    std::lock_guard<std::mutex> lock(g_runtimeMutex);
    MK64_LOGI("nativeInitGame: begin");
    if (!mk64_game_runtime().initialize()) {
        MK64_LOGE("game runtime initialization failed");
        g_stage = RuntimeStage::Failed;
        return JNI_FALSE;
    }
    g_stage = RuntimeStage::GameReady;
    MK64_LOGI("game runtime ready");
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


extern "C" JNIEXPORT jint JNICALL
Java_com_eightcee_mk64_MainActivity_nativeAdvanceFrame(JNIEnv*, jobject) {
    if (g_stage.load() < RuntimeStage::PlatformReady) return 0;
    const unsigned ticks = mk64_host_clock_advance(&g_hostClock, monotonic_now_ns());
    mk64_game_runtime().run_ticks(ticks);
    return static_cast<jint>(ticks);
}

extern "C" JNIEXPORT void JNICALL
Java_com_eightcee_mk64_MainActivity_nativeResumeClock(JNIEnv*, jobject) {
    mk64_host_clock_reset(&g_hostClock, monotonic_now_ns());
}


extern "C" JNIEXPORT void JNICALL
Java_com_eightcee_mk64_MainActivity_nativeSetController(JNIEnv*, jobject,
                                                         jint buttons,
                                                         jint stickX,
                                                         jint stickY) {
    mk64_input_set(static_cast<uint16_t>(buttons), stickX, stickY);
}


extern "C" JNIEXPORT jint JNICALL
Java_com_eightcee_mk64_MainActivity_nativeControllerPacked(JNIEnv*, jobject) {
    const Mk64PadSample pad = mk64_host_sample_pad();
    // Diagnostic bridge: [buttons:16][stickX:8][stickY:8].
    return static_cast<jint>((static_cast<uint32_t>(pad.button) << 16) |
                             (static_cast<uint8_t>(pad.stick_x) << 8) |
                             static_cast<uint8_t>(pad.stick_y));
}


extern "C" JNIEXPORT jint JNICALL
Java_com_eightcee_mk64_MainActivity_nativeRomSize(JNIEnv*, jobject) {
    return static_cast<jint>(g_romImage.size());
}


extern "C" bool mk64_android_pi_read(uintptr_t romAddress, void* destination, size_t size) {
    const bool ok = g_hostPi.dma_read(romAddress, destination, size);
    if (!ok) {
        MK64_LOGE("PI read failed: rom=0x%llx size=%zu",
                  static_cast<unsigned long long>(romAddress), size);
    }
    return ok;
}


extern "C" int mk64_android_dma_copy(void* destination, uintptr_t romAddress, size_t size) {
    return mk64_android_pi_read(romAddress, destination, size) ? 0 : -1;
}


extern "C" JNIEXPORT void JNICALL
Java_com_eightcee_mk64_Mk64Surface_nativeSurfaceCreated(JNIEnv*, jobject) {
    glDisable(GL_DITHER);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    MK64_LOGI("OpenGL ES renderer surface created");
}

extern "C" JNIEXPORT void JNICALL
Java_com_eightcee_mk64_Mk64Surface_nativeSurfaceChanged(JNIEnv*, jobject, jint width, jint height) {
    glViewport(0, 0, width, height);
    mk64_android_gfx_set_size((unsigned)width, (unsigned)height);
    MK64_LOGI("renderer viewport %dx%d", width, height);
}

extern "C" JNIEXPORT void JNICALL
Java_com_eightcee_mk64_Mk64Surface_nativeRenderFrame(JNIEnv*, jobject) {
    if (g_stage.load() >= RuntimeStage::GameReady) {
        const unsigned ticks = mk64_host_clock_advance(&g_hostClock, monotonic_now_ns());
        if (ticks != 0) {
            mk64_game_runtime().run_ticks(ticks);
            g_stage = RuntimeStage::Running;
        }
    } else {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}
