#include "tachyon/runtime/execution/frames/frame_hasher.h"
#include <cstring>

namespace tachyon {

std::uint64_t FrameHasher::hash_pixels(std::span<const std::byte> pixels) noexcept {
    std::uint64_t h = kPrime;
    
    // Process in 64-bit chunks
    const std::size_t n = pixels.size() / sizeof(std::uint64_t);
    const auto* p64 = reinterpret_cast<const std::uint64_t*>(pixels.data());
    
    for (std::size_t i = 0; i < n; ++i) {
        h = mix(h, p64[i]);
    }
    
    // Process remaining bytes
    const std::size_t remaining = pixels.size() % sizeof(std::uint64_t);
    if (remaining > 0) {
        std::uint64_t last = 0;
        std::memcpy(&last, pixels.data() + (n * sizeof(std::uint64_t)), remaining);
        h = mix(h, last);
    }
    
    return h;
}

std::uint64_t FrameHasher::hash_values(std::span<const std::uint64_t> values) noexcept {
    std::uint64_t h = kPrime;
    for (std::uint64_t v : values) {
        h = mix(h, v);
    }
    return h;
}

} // namespace tachyon
