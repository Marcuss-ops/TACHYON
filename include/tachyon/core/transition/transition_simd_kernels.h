#pragma once

#include <cstddef>

namespace tachyon::runtime::simd {

/**
 * @brief Scalar linear blend: out = a * (1 - t) + b * t
 * 
 * Canonical scalar fallback used by TachyonCore.
 */
void lerp_pixels_scalar(float* out, const float* a, const float* b, std::size_t count, float t);

#if defined(TACHYON_ENABLE_HIGHWAY)
void lerp_pixels_highway(float* out, const float* a, const float* b, std::size_t count, float t);
#endif

/**
 * @brief Best available linear blend implementation.
 * 
 * INVARIANT: out (dst), a (src), and b (src) must be at least 16-byte aligned
 * to prevent runtime SIMD instructions faulting/crashing.
 */
void lerp_pixels_best(float* out, const float* a, const float* b, std::size_t count, float t);

} // namespace tachyon::runtime::simd
