#pragma once

#include <cstddef>
#include <cstdint>

namespace tachyon::core::simd {

/**
 * @brief Converts RGBA8 (uint8) pixels to RGBA32F (float32).
 * 
 * @param dst Pointer to destination float buffer (must have size count * 4).
 * @param src Pointer to source uint8 buffer (must have size count * 4).
 * @param pixel_count Number of pixels to convert.
 */
void rgba8_to_float32(float* dst, const std::uint8_t* src, std::size_t pixel_count);

} // namespace tachyon::core::simd
