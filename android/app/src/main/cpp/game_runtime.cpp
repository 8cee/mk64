#include "game_runtime.h"
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "MK64Game", __VA_ARGS__)

#if defined(MK64_GAME_LINKED)
extern "C" void port_game_init(void);
extern "C" void port_game_loop_one_iteration(void);
#endif

Mk64GameRuntime& mk64_game_runtime() {
    static Mk64GameRuntime runtime;
    return runtime;
}

bool Mk64GameRuntime::initialize() {
    if (initialized_.load()) return true;
#if defined(MK64_GAME_LINKED)
    port_game_init();
    initialized_ = true;
    LOGI("MK64 game runtime initialized");
    return true;
#else
    LOGI("MK64 game sources not linked yet");
    return false;
#endif
}

void Mk64GameRuntime::run_ticks(unsigned ticks) {
#if defined(MK64_GAME_LINKED)
    if (!initialized_.load()) return;
    while (ticks--) {
        port_game_loop_one_iteration();
        ++tick_count_;
    }
#else
    (void)ticks;
#endif
}

void Mk64GameRuntime::render() {
    // Game display-list execution is performed by the host renderer from the
    // GL thread. Kept explicit so simulation never owns the Android GL context.
}
