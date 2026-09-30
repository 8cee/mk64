#pragma once
#include <cstdint>

struct Mk64ControllerState {
    uint16_t buttons = 0;
    int8_t stick_x = 0;
    int8_t stick_y = 0;
};

enum Mk64Button : uint16_t {
    MK64_BTN_A      = 0x8000,
    MK64_BTN_B      = 0x4000,
    MK64_BTN_Z      = 0x2000,
    MK64_BTN_START  = 0x1000,
    MK64_BTN_D_UP   = 0x0800,
    MK64_BTN_D_DOWN = 0x0400,
    MK64_BTN_D_LEFT = 0x0200,
    MK64_BTN_D_RIGHT= 0x0100,
    MK64_BTN_L      = 0x0020,
    MK64_BTN_R      = 0x0010,
    MK64_BTN_C_UP   = 0x0008,
    MK64_BTN_C_DOWN = 0x0004,
    MK64_BTN_C_LEFT = 0x0002,
    MK64_BTN_C_RIGHT= 0x0001,
};

void mk64_input_set(uint16_t buttons, int stick_x, int stick_y);
Mk64ControllerState mk64_input_snapshot();
