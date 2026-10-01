#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
typedef unsigned int u32;

static char sSaveDir[1024] = "/data/local/tmp/";
static char sSavePath[1200];

u32 port_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (u32)(((uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL) & 0xffffffffu);
}

const char* port_save_dir(void) {
    return sSaveDir;
}

const char* port_save_path(const char* name) {
    if (!name) return sSaveDir;
    snprintf(sSavePath, sizeof(sSavePath), "%s%s", sSaveDir, name);
    return sSavePath;
}

const char* port_eboot_dir(void) { return sSaveDir; }
void port_set_save_dir(const char* path) {
    if (!path || !*path) return;
    snprintf(sSaveDir, sizeof(sSaveDir), "%s", path);
    size_t n = strlen(sSaveDir);
    if (n && sSaveDir[n - 1] != '/' && n + 1 < sizeof(sSaveDir)) {
        sSaveDir[n] = '/'; sSaveDir[n + 1] = 0;
    }
}
void port_fs_init(void) {}
}

extern "C" {
__attribute__((weak)) float gPortDrawDist = 3000.0f;
__attribute__((weak)) int gfx_trace_frames = 0;
__attribute__((weak)) int gfx_debug_frame = 0;
__attribute__((weak)) int gfx_dump_textures = 0;
__attribute__((weak)) int gPortNoDirectEmit = 0;

__attribute__((weak)) void port_gfx_start_frame(void) {}
__attribute__((weak)) void port_gfx_run(void* dl) { (void)dl; }
__attribute__((weak)) void port_gfx_end_frame(void) {}
__attribute__((weak)) void port_gfx_show_fps_mode(int classic, int saved) { (void)classic; (void)saved; }
__attribute__((weak)) void port_fb_copy_request(int x, int y, int w, int h, unsigned short* target) {
    (void)x; (void)y; (void)w; (void)h; (void)target;
}
__attribute__((weak)) void port_screenshot(int index) { (void)index; }
__attribute__((weak)) void port_depthshot(int index) { (void)index; }
__attribute__((weak)) void port_debug_selftest(void) {}
__attribute__((weak)) void port_debug_frame_begin(unsigned int frame) { (void)frame; }
__attribute__((weak)) void port_debug_frame_end(unsigned int frame) { (void)frame; }
__attribute__((weak)) void port_mirrored_vertices(const void* start, unsigned int bytes) { (void)start; (void)bytes; }
__attribute__((weak)) int port_is_mirrored_vertex(const void* vertex) { (void)vertex; return 0; }
}
