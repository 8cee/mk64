#include "host_controller_adapter.h"

Mk64PadSample mk64_host_sample_pad() {
    const Mk64ControllerState state = mk64_input_snapshot();
    Mk64PadSample pad{};
    pad.button = state.buttons;
    pad.stick_x = state.stick_x;
    pad.stick_y = state.stick_y;
    pad.errno_value = 0;
    return pad;
}
