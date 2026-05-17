#include "tachyon/runtime/execution/frames/frame_hasher.h"
#include "tachyon/core/hash/hash.h"

namespace tachyon {

std::uint64_t FrameHasher::hash_pixels(std::span<const std::byte> pixels) noexcept {
    return hash::hash64(pixels.data(), pixels.size());
}

std::uint64_t FrameHasher::hash_values(std::span<const std::uint64_t> values) noexcept {
    return hash::hash64(values.data(), values.size_bytes());
}

} // namespace tachyon
