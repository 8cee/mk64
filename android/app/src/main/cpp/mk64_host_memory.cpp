#include "mk64_android_dma.h"
#include <stdint.h>
#include <stddef.h>

// Small host-facing equivalents for the data-loading portion of
// src/racing/memory.c. They intentionally avoid RSP/display-list code.

extern "C" int mk64_host_dma_read(uintptr_t rom_start, void* destination, size_t size) {
    if (destination == nullptr || size == 0) return size == 0 ? 0 : -1;
    return mk64_android_dma_copy(destination, rom_start, size);
}

extern "C" int mk64_host_dma_range(uintptr_t rom_start, uintptr_t rom_end, void* destination) {
    if (rom_end < rom_start) return -1;
    return mk64_host_dma_read(rom_start, destination, static_cast<size_t>(rom_end - rom_start));
}
