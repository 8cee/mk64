#include "host_gfx_commands.h"

Mk64GfxCommandKind mk64_gfx_classify(const Mk64GfxCommand& cmd) {
    const uint8_t op = static_cast<uint8_t>(cmd.w0 >> 24);
    // F3DEX-family opcodes used by MK64. The renderer backend will decode
    // payloads separately; classification keeps the execution loop testable.
    switch (op) {
        case 0xB8: return Mk64GfxCommandKind::EndDisplayList;
        case 0x06: return Mk64GfxCommandKind::DisplayList;
        case 0x04: return Mk64GfxCommandKind::Vertex;
        case 0xBF: return Mk64GfxCommandKind::Triangle1;
        case 0xB1: return Mk64GfxCommandKind::Triangle2;
        case 0xFD: return Mk64GfxCommandKind::SetTextureImage;
        case 0xFC: return Mk64GfxCommandKind::SetCombine;
        case 0xB9: return Mk64GfxCommandKind::SetRenderMode;
        case 0xB7: return Mk64GfxCommandKind::SetGeometryMode;
        case 0xB6: return Mk64GfxCommandKind::ClearGeometryMode;
        default: return Mk64GfxCommandKind::Unknown;
    }
}
