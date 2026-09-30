#include "host_segments.h"

void mk64_segment_set(Mk64SegmentTable* table, unsigned segment, uintptr_t base) {
    if (!table || segment >= 16) return;
    table->base[segment] = base;
}

void* mk64_segment_resolve(const Mk64SegmentTable* table, uintptr_t address) {
    if (!table) return nullptr;
#if UINTPTR_MAX > 0xffffffffu
    // Native Android pointers do not fit the N64 0xSSOOOOOO segmented form.
    if (address > 0xffffffffu) return reinterpret_cast<void*>(address);
#endif
    const unsigned segment = static_cast<unsigned>((address >> 24) & 0x0f);
    const uintptr_t offset = address & 0x00ffffffu;
    const uintptr_t base = table->base[segment];
    if (base == 0) return nullptr;
    return reinterpret_cast<void*>(base + offset);
}
