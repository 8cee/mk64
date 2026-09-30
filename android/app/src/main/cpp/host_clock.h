#pragma once
#include <cstdint>

struct Mk64HostClock {
    uint64_t accumulator_ns;
    uint64_t last_ns;
    uint64_t tick_count;
    bool initialized;
};

void mk64_host_clock_reset(Mk64HostClock* clock, uint64_t now_ns);
unsigned mk64_host_clock_advance(Mk64HostClock* clock, uint64_t now_ns);
