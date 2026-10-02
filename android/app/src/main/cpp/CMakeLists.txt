cmake_minimum_required(VERSION 3.22.1)
project(mk64_android LANGUAGES C CXX)

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
    gfx_ptr_android.cpp
    android_gfx_window.cpp
    portable/gfx/gfx_pc_android.c
    portable/gfx/gfx_gles2.c
    game_runtime.cpp
    portable/ultra_shim.c
    ../../../../../src/port/segments.c
    ../../../../../src/port/decoders.c
    ../../../../../src/port/tkmk00_port.c
    ../../../../../src/port/gfx/gfx_cc.c
    controller_android.cpp
    android_port_services.cpp
    android_crash_stub.c
    android_asset_bss_stubs.c
    android_assets.cpp
    android_recipe_blob.c
    generated_asset_symbols.c
    android_port_seg_tables.c
    ../../../../../src/main.c
    ../../../../../courses/courseTable.c
)

file(GLOB MK64_GAME_ROOT_C "../../../../../src/*.c")
file(GLOB MK64_GAME_BUFFERS_C "../../../../../src/buffers/*.c")
file(GLOB MK64_GAME_RACING_C "../../../../../src/racing/*.c")
file(GLOB MK64_GAME_ENDING_C "../../../../../src/ending/*.c")
file(GLOB MK64_GAME_AUDIO_C "../../../../../src/audio/*.c")
file(GLOB MK64_GAME_DATA_C "../../../../../src/data/*.c")
file(GLOB MK64_GU_C "../../../../../src/os/gu*.c")

list(FILTER MK64_GAME_ROOT_C EXCLUDE REGEX "/main\\.c$")
list(FILTER MK64_GAME_ROOT_C EXCLUDE REGEX "/crash_screen\\.c$")
list(FILTER MK64_GAME_ROOT_C EXCLUDE REGEX "\\.inc\\.c$")
list(FILTER MK64_GAME_BUFFERS_C EXCLUDE REGEX "\\.inc\\.c$")
list(FILTER MK64_GAME_RACING_C EXCLUDE REGEX "\\.inc\\.c$")
list(FILTER MK64_GAME_ENDING_C EXCLUDE REGEX "\\.inc\\.c$")
list(FILTER MK64_GAME_AUDIO_C EXCLUDE REGEX "\\.inc\\.c$")
list(FILTER MK64_GAME_DATA_C EXCLUDE REGEX "\\.inc\\.c$")
list(FILTER MK64_GAME_DATA_C EXCLUDE REGEX "/data_segment2\\.c$")
list(FILTER MK64_GAME_DATA_C EXCLUDE REGEX "/path_spawn_metadata\\.c$")
list(FILTER MK64_GAME_DATA_C EXCLUDE REGEX "/some_data\\.c$")
list(FILTER MK64_GAME_DATA_C EXCLUDE REGEX "/textures\\.c$")

target_sources(mk64_android PRIVATE
    ${MK64_GAME_ROOT_C}
    ${MK64_GAME_BUFFERS_C}
    ${MK64_GAME_RACING_C}
    ${MK64_GAME_ENDING_C}
    ${MK64_GAME_AUDIO_C}
    ${MK64_GAME_DATA_C}
    ${MK64_GU_C}
    ../../../../../src/os/math/sinf.c
    ../../../../../src/os/math/cosf.c
)

target_compile_features(mk64_android PRIVATE cxx_std_17)
find_library(log_lib log)
find_library(gles2_lib GLESv2)
target_link_libraries(mk64_android ${log_lib} ${gles2_lib})

# Android devices using 16 KiB memory pages require native ELF LOAD segments
# to be aligned accordingly. Keep this compatible with both 4 KiB and 16 KiB
# page-size devices.
target_link_options(mk64_android PRIVATE
    "-Wl,-z,max-page-size=16384"
    "-Wl,-z,common-page-size=16384"
)

target_compile_definitions(mk64_android PRIVATE VERSION_US=1 F3DEX_GBI=1 F3D_OLD=1 NON_MATCHING=1 AVOID_UB=1 _LANGUAGE_C=1 NO_SEGMENTED_MEMORY=1 TARGET_ANDROID=1 PORT_ENABLE_AUDIO=0 MK64_GAME_LINKED=1 PORT_DRAW_DIST=3000)
target_include_directories(mk64_android PRIVATE ../../../../../ ../../../../../include ../../../../../src ../../../../../src/racing ../../../../../src/ending ../../../../../src/port ../../../../../src/port/gfx . portable portable/gfx)
