#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace tachyon::core::media {

struct TextureCompressionInput {
    int width = 0;
    int height = 0;
    int channels = 4;
    std::vector<std::uint8_t> rgba;
};

struct TextureCompressionOutput {
    std::vector<std::uint8_t> bytes;
    std::string codec = "none";
};

class TextureCompressor {
public:
    virtual ~TextureCompressor() = default;
    virtual TextureCompressionOutput compress(const TextureCompressionInput& input) = 0;
};

} // namespace tachyon::core::media
