#include "tachyon/core/transition/transition_simd_kernels.h"

#include <immintrin.h>

namespace tachyon::runtime::simd {

void lerp_pixels_avx2(float* out, const float* a, const float* b, std::size_t count, float t) {
    const __m256 vt = _mm256_set1_ps(t);
    const __m256 v_inv_t = _mm256_set1_ps(1.0f - t);

    std::size_t i = 0;
    // Process 8 floats at a time
    for (; i + 7 < count; i += 8) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);

        __m256 res = _mm256_add_ps(
            _mm256_mul_ps(va, v_inv_t),
            _mm256_mul_ps(vb, vt)
        );

        _mm256_storeu_ps(out + i, res);
    }

    // Scalar fallback for remaining elements
    const float inv_t = 1.0f - t;
    for (; i < count; ++i) {
        out[i] = a[i] * inv_t + b[i] * t;
    }
}

} // namespace tachyon::runtime::simd
