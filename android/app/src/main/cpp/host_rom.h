#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class Mk64RomOrder { Z64, V64, N64 };

class Mk64RomImage {
public:
    bool load(const std::string& path, std::string* error);
    bool read(size_t offset, void* dst, size_t size) const;
    void clear();
    size_t size() const { return bytes_.size(); }
    const uint8_t* data() const { return bytes_.data(); }
private:
    std::vector<uint8_t> bytes_;
};
