#pragma once
#include <cstdint>
#include <cstddef>

struct Mk64SegmentTable {
    uintptr_t base[16]{};
};

void mk64_segment_set(Mk64SegmentTable* table, unsigned segment, uintptr_t base);
void* mk64_segment_resolve(const Mk64SegmentTable* table, uintptr_t address);
