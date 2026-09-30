#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Host equivalent of libultra cartridge DMA. Returns 0 on success.
int mk64_android_dma_copy(void* destination, uintptr_t rom_address, size_t size);

#ifdef __cplusplus
}
#endif
