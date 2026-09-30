#include "host_pi.h"

namespace {
constexpr uintptr_t kCartBase = 0x10000000u;
}

bool Mk64HostPi::dma_read(uintptr_t rom_address, void* destination, size_t size) const {
    if (!rom_) return false;
    // Decomp symbols may be expressed either as raw ROM offsets or as
    // cartridge-domain addresses. Normalize both forms to a file offset.
    size_t offset;
    if (rom_address >= kCartBase) {
        offset = static_cast<size_t>(rom_address - kCartBase);
    } else {
        offset = static_cast<size_t>(rom_address);
    }
    return rom_->read(offset, destination, size);
}
