#include <stdint.h>
#include <stddef.h>
#include <atomic>

extern "C" {
extern uintptr_t gSegmentTable[16];
}

namespace {
constexpr uint32_t kTokenPrefix = 0xFE000000u;
constexpr uint32_t kTokenMask   = 0x00FFFFFFu;
constexpr uint32_t kTableSize   = 65536u;

struct Entry {
    std::atomic<uintptr_t> ptr;
};

Entry gEntries[kTableSize];
std::atomic<uint32_t> gNext{1u};

static inline void* module_low32_resolve(uint32_t token) {
#if UINTPTR_MAX > UINT32_MAX
    const uintptr_t anchor = reinterpret_cast<uintptr_t>(&module_low32_resolve);
    const uintptr_t high = anchor & ~uintptr_t(0xFFFFFFFFu);
    return reinterpret_cast<void*>(high | uintptr_t(token));
#else
    return reinterpret_cast<void*>(uintptr_t(token));
#endif
}
}

extern "C" uint32_t port_gfx_ptr_token(const void* p) {
    const uintptr_t full = reinterpret_cast<uintptr_t>(p);
    if (full == 0) {
        return 0;
    }

    // Native N64/segment/token values already fit in 32 bits. Preserve them.
    if (full <= UINT32_MAX) {
        return static_cast<uint32_t>(full);
    }

    // Reuse an existing mapping when possible. The table is deliberately small
    // because MK64's per-frame pointers come from stable pools and repeat.
    for (uint32_t i = 1; i < kTableSize; ++i) {
        if (gEntries[i].ptr.load(std::memory_order_relaxed) == full) {
            return kTokenPrefix | i;
        }
    }

    uint32_t slot = gNext.fetch_add(1u, std::memory_order_relaxed);
    if (slot >= kTableSize) {
        // Wrap and look for an empty slot. This should be rare because runtime
        // display-list pointers are heavily reused.
        for (uint32_t i = 1; i < kTableSize; ++i) {
            uintptr_t expected = 0;
            if (gEntries[i].ptr.compare_exchange_strong(expected, full, std::memory_order_relaxed)) {
                return kTokenPrefix | i;
            }
        }
        // Last-resort low-32 token. The resolver can reconstruct module pointers.
        return static_cast<uint32_t>(full);
    }

    gEntries[slot].ptr.store(full, std::memory_order_relaxed);
    return kTokenPrefix | slot;
}

extern "C" void* port_gfx_ptr_resolve(uint32_t token) {
    if (token == 0) {
        return nullptr;
    }

    if ((token & 0xFF000000u) == kTokenPrefix) {
        const uint32_t slot = token & kTokenMask;
        if (slot < kTableSize) {
            const uintptr_t p = gEntries[slot].ptr.load(std::memory_order_relaxed);
            if (p != 0) {
                return reinterpret_cast<void*>(p);
            }
        }
        return nullptr;
    }

    // Live N64 segmented addresses belong to the segment resolver, not here.
    const uint32_t seg = token >> 24;
    if (seg < 16 && gSegmentTable[seg] != 0) {
        return nullptr;
    }

    // Static display-list initializers on LP64 store the low 32 bits of a
    // module pointer. Android loads libmk64_android.so within one 4 GiB window,
    // so restore the shared object's high half from this function's address.
    return module_low32_resolve(token);
}
