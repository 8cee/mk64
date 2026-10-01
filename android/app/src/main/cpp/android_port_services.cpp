#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {
typedef unsigned long uintptr_t;
typedef unsigned int u32;

uintptr_t gSegmentTable[16] = {0};

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
