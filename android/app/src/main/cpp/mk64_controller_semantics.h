#pragma once
#include <cstdint>

struct Mk64ControllerEdges {
    uint16_t button = 0;
    uint16_t button_pressed = 0;
    uint16_t button_depressed = 0;
    int8_t raw_stick_x = 0;
    int8_t raw_stick_y = 0;
    uint16_t stick_direction = 0;
    uint16_t stick_pressed = 0;
    uint16_t stick_depressed = 0;
};

void mk64_host_update_controller(Mk64ControllerEdges* controller);
