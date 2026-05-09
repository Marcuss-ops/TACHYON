#pragma once

#include <cstddef>

namespace tachyon::runtime::simd {

/**
 * @brief Scalar linear blend: out = a * (1 - t) + b * t
 */
void lerp_pixels_scalar(float* out, const float* a, const float* b, std::size_t count, float t);

/**
 * @brief Check if AVX2 is available on the current CPU.
 */
bool avx2_available();

/**
 * @brief AVX2 linear blend: out = a * (1 - t) + b * t
 *
 * Requires AVX2 support. Call avx2_available() first.
 */
void lerp_pixels_avx2(float* out, const float* a, const float* b, std::size_t count, float t);

/**
 * @brief Best-available linear blend dispatch.
 *
 * Uses AVX2 if available at runtime, otherwise falls back to scalar.
 */
void lerp_pixels_best(float* out, const float* a, const float* b, std::size_t count, float t);

} // namespace tachyon::runtime::simd