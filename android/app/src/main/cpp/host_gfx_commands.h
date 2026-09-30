#pragma once
#include <cstddef>
#include <cstdint>

enum class Mk64GfxCommandKind {
    Unknown, EndDisplayList, DisplayList, Vertex, Triangle1, Triangle2,
    SetTextureImage, SetCombine, SetRenderMode, SetGeometryMode, ClearGeometryMode
};

struct Mk64GfxCommand {
    uint32_t w0;
    uint32_t w1;
};

Mk64GfxCommandKind mk64_gfx_classify(const Mk64GfxCommand& cmd);
