/**
 * @file frame_hasher.h
 * @brief High-performance pixel and command buffer hashing.
 */

#pragma once

#include "tachyon/core/api.h"
#include <cstdint>
#include <span>
#include <vector>

namespace tachyon {

/**
 * @brief Utility for generating deterministic fingerprints of render outputs.
 * Uses a modified 64-bit mixer designed for high throughput.
 */
class TACHYON_API FrameHasher {
public:
    FrameHasher() noexcept = default;

    /**
     * @brief Hashes a raw pixel buffer (RGBA8/RGBAF32).
     */
    [[nodiscard]] std::uint64_t hash_pixels(std::span<const std::byte> pixels) noexcept;
    [[nodiscard]] std::uint64_t hash_pixels(std::span<const std::uint32_t> pixels) noexcept {
        return hash_pixels(std::as_bytes(pixels));
    }
    [[nodiscard]] std::uint64_t hash_pixels(std::span<const float> pixels) noexcept {
        return hash_pixels(std::as_bytes(pixels));
    }

    /**
     * @brief Hashes a sequence of 64-bit values (e.g. command hashes).
     */
    [[nodiscard]] std::uint64_t hash_values(std::span<const std::uint64_t> values) noexcept;

private:
    static constexpr std::uint64_t kPrime = 0x9e3779b97f4a7c15ULL;
    
    [[nodiscard]] static constexpr std::uint64_t mix(std::uint64_t h, std::uint64_t v) noexcept {
        h ^= v + kPrime + (h << 6U) + (h >> 2U);
        return h;
    }
};

} // namespace tachyon
