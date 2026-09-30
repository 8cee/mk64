#include "host_rom.h"
#include <algorithm>
#include <cstring>
#include <fstream>

namespace {
bool detect_order(const std::vector<uint8_t>& b, Mk64RomOrder* order) {
    if (b.size() < 4) return false;
    if (b[0]==0x80 && b[1]==0x37 && b[2]==0x12 && b[3]==0x40) { *order=Mk64RomOrder::Z64; return true; }
    if (b[0]==0x37 && b[1]==0x80 && b[2]==0x40 && b[3]==0x12) { *order=Mk64RomOrder::V64; return true; }
    if (b[0]==0x40 && b[1]==0x12 && b[2]==0x37 && b[3]==0x80) { *order=Mk64RomOrder::N64; return true; }
    return false;
}
void normalize(std::vector<uint8_t>& b, Mk64RomOrder order) {
    if (order == Mk64RomOrder::V64) {
        for (size_t i=0; i+1<b.size(); i+=2) std::swap(b[i], b[i+1]);
    } else if (order == Mk64RomOrder::N64) {
        for (size_t i=0; i+3<b.size(); i+=4) {
            std::swap(b[i], b[i+3]); std::swap(b[i+1], b[i+2]);
        }
    }
}
}

bool Mk64RomImage::load(const std::string& path, std::string* error) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) { if(error)*error="unable to open ROM"; return false; }
    const auto end=f.tellg();
    if (end <= 0) { if(error)*error="empty ROM"; return false; }
    bytes_.resize(static_cast<size_t>(end));
    f.seekg(0);
    if (!f.read(reinterpret_cast<char*>(bytes_.data()), bytes_.size())) { clear(); if(error)*error="ROM read failed"; return false; }
    Mk64RomOrder order;
    if (!detect_order(bytes_, &order)) { clear(); if(error)*error="unsupported N64 byte order"; return false; }
    normalize(bytes_, order);
    if (bytes_.size() < 0x40 || std::memcmp(bytes_.data()+0x3B, "NKTE", 4) != 0) {
        clear(); if(error)*error="ROM is not Mario Kart 64 USA (NKTE)"; return false;
    }
    return true;
}

bool Mk64RomImage::read(size_t offset, void* dst, size_t size) const {
    if (!dst || offset > bytes_.size() || size > bytes_.size()-offset) return false;
    std::memcpy(dst, bytes_.data()+offset, size);
    return true;
}
void Mk64RomImage::clear() { bytes_.clear(); bytes_.shrink_to_fit(); }
