#pragma once
#include <cstdint>
#include "host_input.h"

struct Mk64PadSample {
    uint16_t button;
    int8_t stick_x;
    int8_t stick_y;
    uint8_t errno_value;
};

Mk64PadSample mk64_host_sample_pad();
