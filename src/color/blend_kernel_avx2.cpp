#include "tachyon/color/blend_kernel.h"
#include <immintrin.h>

namespace tachyon::color::kernel {

#ifdef TACHYON_AVX2

void additive_avx2(const LinearRGBA* src, LinearRGBA* dst, std::size_t count) {
    std::size_t i = 0;
    // Process 2 pixels at a time (2 * 4 * sizeof(float) = 32 bytes)
    for (; i + 1 < count; i += 2) {
        __m256 src_vec = _mm256_loadu_ps(reinterpret_cast<const float*>(&src[i]));
        __m256 dst_vec = _mm256_loadu_ps(reinterpret_cast<const float*>(&dst[i]));

        // dst = dst + src (Additive)
        __m256 res = _mm256_add_ps(src_vec, dst_vec);
        _mm256_storeu_ps(reinterpret_cast<float*>(&dst[i]), res);
    }
    
    // Fallback for remaining pixels
    for (; i < count; ++i) {
        dst[i].r += src[i].r;
        dst[i].g += src[i].g;
        dst[i].b += src[i].b;
        dst[i].a += src[i].a;
    }
}

#endif

} // namespace tachyon::color::kernel
