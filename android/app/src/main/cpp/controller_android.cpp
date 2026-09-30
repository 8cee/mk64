#include "host_input.h"
extern "C" {
#include <ultra64.h>

void controller_android_init(void) {}

void controller_android_read(OSContPad* pad) {
    Mk64ControllerState s = mk64_input_snapshot();
    pad->button = s.buttons;
    pad->stick_x = s.stick_x;
    pad->stick_y = s.stick_y;
    pad->errno = 0;
}
}
