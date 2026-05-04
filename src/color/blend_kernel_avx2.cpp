#include "tachyon/color/blend_kernel.h"
#include <immintrin.h>
#include <algorithm>

namespace tachyon::color::kernel {

#ifdef TACHYON_AVX2

// Helper to broadcast alpha channel from RGBA-AOS layout to all channels
// Input:  [R0, G0, B0, A0, R1, G1, B1, A1]
// Output: [A0, A0, A0, A0, A1, A1, A1, A1]
inline __m256 broadcast_alpha(__m256 rgba) {
    return _mm256_permute_ps(rgba, _MM_SHUFFLE(3, 3, 3, 3));
}

void normal_avx2(const LinearRGBA* src, LinearRGBA* dst, std::size_t count) {
    std::size_t i = 0;
    const __m256 one = _mm256_set1_ps(1.0f);
    const __m256 zero = _mm256_setzero_ps();

    for (; i + 1 < count; i += 2) {
        __m256 s = _mm256_loadu_ps(reinterpret_cast<const float*>(&src[i]));
        __m256 d = _mm256_loadu_ps(reinterpret_cast<const float*>(&dst[i]));

        __m256 sa = broadcast_alpha(s);
        __m256 da = broadcast_alpha(d);
        __m256 inv_sa = _mm256_sub_ps(one, sa);

        // out_a = sa + da * (1 - sa)
        __m256 out_a = _mm256_add_ps(sa, _mm256_mul_ps(da, inv_sa));
        
        // out_rgb = (src_rgb * sa + dst_rgb * da * (1 - sa)) / out_a
        __m256 term1 = _mm256_mul_ps(s, sa);
        __m256 term2 = _mm256_mul_ps(_mm256_mul_ps(d, da), inv_sa);
        __m256 out_rgb = _mm256_div_ps(_mm256_add_ps(term1, term2), out_a);

        // Handle alpha separately (formula for alpha is different but we can just use out_a for the 4th channel)
        // Actually, out_rgb calculated above already has the correct alpha in the 4th channel if we consider s_a * s_a + ...
        // Wait, the alpha channel of out_rgb would be (sa*sa + da*da*inv_sa)/out_a. That's NOT out_a.
        // Let's fix the alpha channel in the result.
        
        // Mask to keep RGB from division, but take A from out_a
        // (Alternatively, use a simpler formula if we know it's pre-multiplied, but core uses straight)
        
        // Correct approach: Blend RGB, then blend A.
        // We use a blend mask to pick the 4th channel from out_a.
        // 0x88 -> 1000 1000 (bits for 4th and 8th float)
        __m256 res = _mm256_blend_ps(out_rgb, out_a, 0x88);

        // Guard against out_a <= 0
        __m256 a_zero_mask = _mm256_cmp_ps(out_a, zero, _CMP_LE_OQ);
        res = _mm256_andnot_ps(a_zero_mask, res);

        _mm256_storeu_ps(reinterpret_cast<float*>(&dst[i]), res);
    }

    // Fallback
    for (; i < count; ++i) {
        float sa = src[i].a;
        float da = dst[i].a;
        float out_a = sa + da * (1.0f - sa);
        if (out_a <= 0.0f) {
            dst[i] = {0,0,0,0};
            continue;
        }
        dst[i].r = (src[i].r * sa + dst[i].r * da * (1.0f - sa)) / out_a;
        dst[i].g = (src[i].g * sa + dst[i].g * da * (1.0f - sa)) / out_a;
        dst[i].b = (src[i].b * sa + dst[i].b * da * (1.0f - sa)) / out_a;
        dst[i].a = out_a;
    }
}

void additive_avx2(const LinearRGBA* src, LinearRGBA* dst, std::size_t count) {
    std::size_t i = 0;
    const __m256 one = _mm256_set1_ps(1.0f);

    for (; i + 1 < count; i += 2) {
        __m256 s = _mm256_loadu_ps(reinterpret_cast<const float*>(&src[i]));
        __m256 d = _mm256_loadu_ps(reinterpret_cast<const float*>(&dst[i]));
        __m256 res = _mm256_add_ps(s, d);
        res = _mm256_min_ps(res, one);
        _mm256_storeu_ps(reinterpret_cast<float*>(&dst[i]), res);
    }
    
    for (; i < count; ++i) {
        dst[i].r = std::min(1.0f, src[i].r + dst[i].r);
        dst[i].g = std::min(1.0f, src[i].g + dst[i].g);
        dst[i].b = std::min(1.0f, src[i].b + dst[i].b);
        dst[i].a = std::min(1.0f, src[i].a + dst[i].a);
    }
}

void multiply_avx2(const LinearRGBA* src, LinearRGBA* dst, std::size_t count) {
    std::size_t i = 0;
    for (; i + 1 < count; i += 2) {
        __m256 s = _mm256_loadu_ps(reinterpret_cast<const float*>(&src[i]));
        __m256 d = _mm256_loadu_ps(reinterpret_cast<const float*>(&dst[i]));
        __m256 res = _mm256_mul_ps(s, d);
        _mm256_storeu_ps(reinterpret_cast<float*>(&dst[i]), res);
    }
    for (; i < count; ++i) {
        dst[i].r *= src[i].r;
        dst[i].g *= src[i].g;
        dst[i].b *= src[i].b;
        dst[i].a *= src[i].a;
    }
}

void screen_avx2(const LinearRGBA* src, LinearRGBA* dst, std::size_t count) {
    std::size_t i = 0;
    const __m256 one = _mm256_set1_ps(1.0f);
    for (; i + 1 < count; i += 2) {
        __m256 s = _mm256_loadu_ps(reinterpret_cast<const float*>(&src[i]));
        __m256 d = _mm256_loadu_ps(reinterpret_cast<const float*>(&dst[i]));
        // res = s + d - s * d
        __m256 res = _mm256_sub_ps(_mm256_add_ps(s, d), _mm256_mul_ps(s, d));
        _mm256_storeu_ps(reinterpret_cast<float*>(&dst[i]), res);
    }
    for (; i < count; ++i) {
        dst[i].r = src[i].r + dst[i].r - src[i].r * dst[i].r;
        dst[i].g = src[i].g + dst[i].g - src[i].g * dst[i].g;
        dst[i].b = src[i].b + dst[i].b - src[i].b * dst[i].b;
        dst[i].a = src[i].a + dst[i].a - src[i].a * dst[i].a;
    }
}

#endif

} // namespace tachyon::color::kernel
