#include "host_clock.h"

namespace {
constexpr uint64_t kTickNs = 1000000000ULL / 30ULL;
constexpr unsigned kMaxCatchupTicks = 5;
}

void mk64_host_clock_reset(Mk64HostClock* clock, uint64_t now_ns) {
    clock->accumulator_ns = 0;
    clock->last_ns = now_ns;
    clock->tick_count = 0;
    clock->initialized = true;
}

unsigned mk64_host_clock_advance(Mk64HostClock* clock, uint64_t now_ns) {
    if (!clock->initialized || now_ns < clock->last_ns) {
        mk64_host_clock_reset(clock, now_ns);
        return 0;
    }
    uint64_t elapsed = now_ns - clock->last_ns;
    clock->last_ns = now_ns;
    const uint64_t maxElapsed = kTickNs * kMaxCatchupTicks;
    if (elapsed > maxElapsed) elapsed = maxElapsed;
    clock->accumulator_ns += elapsed;

    unsigned ticks = 0;
    while (clock->accumulator_ns >= kTickNs && ticks < kMaxCatchupTicks) {
        clock->accumulator_ns -= kTickNs;
        ++clock->tick_count;
        ++ticks;
    }
    return ticks;
}
