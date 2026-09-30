#pragma once
#include <cstddef>
#include <cstdint>
#include "host_rom.h"

class Mk64HostPi {
public:
    explicit Mk64HostPi(const Mk64RomImage* rom) : rom_(rom) {}
    bool dma_read(uintptr_t rom_address, void* destination, size_t size) const;
private:
    const Mk64RomImage* rom_;
};
