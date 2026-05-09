#pragma once
#include <cstddef>
#include <immintrin.h>

namespace tachyon::runtime::simd {

/**
 * @brief Performs a linear blend (lerp) between two buffers using AVX2.
 * pixels_out = pixels_a * (1 - t) + pixels_b * t
 * @param pixels_out Destination buffer (must be large enough)
 * @param pixels_a Source buffer A
 * @param pixels_b Source buffer B
 * @param count Number of float values to process
 * @param t Blend factor [0, 1]
 */
inline void lerp_pixels_avx2(float* pixels_out, const float* pixels_a, const float* pixels_b, std::size_t count, float t) {
    const __m256 vt = _mm256_set1_ps(t);
    const __m256 v_inv_t = _mm256_set1_ps(1.0f - t);

    std::size_t i = 0;
    // Process 8 floats at a time
    for (; i + 7 < count; i += 8) {
        __m256 va = _mm256_loadu_ps(pixels_a + i);
        __m256 vb = _mm256_loadu_ps(pixels_b + i);
        
        __m256 res = _mm256_add_ps(
            _mm256_mul_ps(va, v_inv_t),
            _mm256_mul_ps(vb, vt)
        );
        
        _mm256_storeu_ps(pixels_out + i, res);
    }

    // Scalar fallback for remaining elements
    const float inv_t = 1.0f - t;
    for (; i < count; ++i) {
        pixels_out[i] = pixels_a[i] * inv_t + pixels_b[i] * t;
    }
}

} // namespace tachyon::runtime::simd
