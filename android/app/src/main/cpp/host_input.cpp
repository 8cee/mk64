#include "host_input.h"
#include <algorithm>
#include <mutex>

namespace {
std::mutex g_inputMutex;
Mk64ControllerState g_input;
}

void mk64_input_set(uint16_t buttons, int stick_x, int stick_y) {
    std::lock_guard<std::mutex> lock(g_inputMutex);
    g_input.buttons = buttons;
    g_input.stick_x = static_cast<int8_t>(std::clamp(stick_x, -80, 80));
    g_input.stick_y = static_cast<int8_t>(std::clamp(stick_y, -80, 80));
}

Mk64ControllerState mk64_input_snapshot() {
    std::lock_guard<std::mutex> lock(g_inputMutex);
    return g_input;
}
