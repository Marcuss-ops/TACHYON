#pragma once

#include "tachyon/color/color_space.h"
#include <cstddef>

namespace tachyon::color {

/**
 * @brief High-performance kernels for color blending.
 * These kernels operate on spans of pixels and are optimized with SIMD (AVX2/NEON)
 * where available.
 */
namespace kernel {

void normal(const LinearRGBA* src, LinearRGBA* dst, std::size_t count);
void additive(const LinearRGBA* src, LinearRGBA* dst, std::size_t count);
void multiply(const LinearRGBA* src, LinearRGBA* dst, std::size_t count);
void screen(const LinearRGBA* src, LinearRGBA* dst, std::size_t count);

} // namespace kernel

} // namespace tachyon::color
