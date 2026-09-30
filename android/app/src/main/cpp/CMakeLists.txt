cmake_minimum_required(VERSION 3.22.1)
project(mk64_android LANGUAGES C CXX)

# Android native runtime bridge. MK64 game subsystems are brought into this
# library incrementally while libultra services are replaced by host services.
add_library(mk64_android SHARED
    native_bridge.cpp
    host_clock.cpp
    host_input.cpp
    host_controller_adapter.cpp
    mk64_controller_semantics.cpp
    host_rom.cpp
    host_pi.cpp
    mk64_host_memory.cpp
    host_segments.cpp
    host_message_queue.cpp
    host_gfx_commands.cpp
    game_runtime.cpp
    android_gfx_api.cpp
    portable/gfx/gfx_cc.c
)

target_compile_features(mk64_android PRIVATE cxx_std_17)
find_library(log_lib log)
find_library(gles2_lib GLESv2)
target_link_libraries(mk64_android ${log_lib} ${gles2_lib})
