#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool mk64_android_pi_read(uintptr_t rom_address, void* destination, size_t size);

#ifdef __cplusplus
}
#endif
