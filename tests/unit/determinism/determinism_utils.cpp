#include "determinism_utils.h"

namespace tachyon::test {

std::uint64_t hash_rgba8_bytes(const std::vector<std::uint8_t>& bytes) {
    std::uint64_t hash = 14695981039346656037ull;
    for (std::uint8_t byte : bytes) {
        hash ^= static_cast<std::uint64_t>(byte);
        hash *= 1099511628211ull;
    }
    return hash;
}

std::uint64_t hash_frame_rgba8(const renderer2d::Framebuffer& frame) {
    std::vector<std::uint8_t> pixels;
    frame.to_rgba8(pixels);
    return hash_rgba8_bytes(pixels);
}

}
