#include "mk64_controller_semantics.h"
#include "host_controller_adapter.h"

namespace {
constexpr uint16_t L_JPAD = 0x0200;
constexpr uint16_t R_JPAD = 0x0100;
constexpr uint16_t D_JPAD = 0x0400;
constexpr uint16_t U_JPAD = 0x0800;
constexpr uint16_t D_CBUTTONS = 0x0004;
constexpr uint16_t Z_TRIG = 0x2000;
}

void mk64_host_update_controller(Mk64ControllerEdges* controller) {
    const Mk64PadSample pad = mk64_host_sample_pad();
    uint16_t buttons = pad.button;
    if ((buttons & D_CBUTTONS) != 0) buttons |= Z_TRIG;

    controller->raw_stick_x = pad.stick_x;
    controller->raw_stick_y = pad.stick_y;
    controller->button_pressed = buttons & (buttons ^ controller->button);
    controller->button_depressed = controller->button & (buttons ^ controller->button);
    controller->button = buttons;

    uint16_t stick = 0;
    if (pad.stick_x < -50) stick |= L_JPAD;
    if (pad.stick_x > 50) stick |= R_JPAD;
    if (pad.stick_y < -50) stick |= D_JPAD;
    if (pad.stick_y > 50) stick |= U_JPAD;
    controller->stick_pressed = stick & (stick ^ controller->stick_direction);
    controller->stick_depressed = controller->stick_direction & (stick ^ controller->stick_direction);
    controller->stick_direction = stick;
}
