#include <atomic>
#include <chrono>
extern "C" {
#include "portable/gfx/gfx_pc.h"
#include "portable/gfx/gfx_rendering_api.h"
#include "portable/gfx/gfx_window_manager_api.h"
#include <PR/gbi.h>

extern struct GfxRenderingAPI mk64_android_gfx_api;
void mk64_android_gfx_context_lost(void);

static std::atomic<unsigned> sWidth{1280};
static std::atomic<unsigned> sHeight{720};
static bool sGfxInitialized = false;

static void wm_init(const char*, bool) {}
static void wm_keys(bool (*)(int), bool (*)(int), void (*)(void)) {}
static void wm_full_cb(void (*)(bool)) {}
static void wm_full(bool) {}
static void wm_loop(void (*run)(void)) { if (run) run(); }
static void wm_dims(uint32_t* w, uint32_t* h) {
    if (w) *w = sWidth.load();
    if (h) *h = sHeight.load();
}
static void wm_events(void) {}
static bool wm_start(void) { return true; }
static void wm_swap_begin(void) {}
static void wm_swap_end(void) {}
static double wm_time(void) {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

static struct GfxWindowManagerAPI sAndroidWm = {
    wm_init, wm_keys, wm_full_cb, wm_full, wm_loop, wm_dims, wm_events,
    wm_start, wm_swap_begin, wm_swap_end, wm_time
};

void mk64_android_gfx_set_size(unsigned w, unsigned h) {
    if (w) sWidth = w;
    if (h) sHeight = h;
}

void mk64_android_gfx_surface_created(void) {
    if (sGfxInitialized) {
        mk64_android_gfx_context_lost();
        sGfxInitialized = false;
    }
}

void mk64_android_gfx_ensure_init(void) {
    if (!sGfxInitialized) {
        gfx_init(&sAndroidWm, &mk64_android_gfx_api, "Mario Kart 64", false);
        sGfxInitialized = true;
    }
}

void port_gfx_start_frame(void) {
    mk64_android_gfx_ensure_init();
    gfx_start_frame();
}
void port_gfx_run(Gfx* dl) {
    mk64_android_gfx_ensure_init();
    gfx_run(dl);
}
void port_gfx_end_frame(void) {
    gfx_end_frame();
}
}
