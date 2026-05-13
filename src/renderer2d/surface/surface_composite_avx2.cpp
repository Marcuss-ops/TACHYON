#include <immintrin.h>
#include <cstddef>
#include <cstdint>

// Compiled with /arch:AVX2 (MSVC) or -mavx2 (GCC/Clang).
// Called from surface_composite.cpp when TACHYON_AVX2 is defined.

namespace tachyon::renderer2d {

// Straight-alpha Porter-Duff "over" for two RGBA-interleaved float pixels per
// AVX2 iteration.
//
// Pixel layout: [R, G, B, A] x N, one __m256 = 2 pixels = 8 floats.
//
// Formula per pixel (straight alpha):
//   out.rgb = src.rgb * src.a + dst.rgb * (1 - src.a)
//   out.a   = src.a           + dst.a   * (1 - src.a)
//
// AVX2 implementation:
//   sa       = permute(src, 0xFF)        -> [a0,a0,a0,a0, a1,a1,a1,a1]
//   inv_sa   = 1.0 - sa
//   s_premul = src * sa                 -> rgb premultiplied, but a channel = a*a
//   s_premul = blend(s_premul, src, 0x88) -> fix alpha positions back to src.a
//   result   = fmadd(dst, inv_sa, s_premul)

void composite_tile_avx2(
    float* __restrict dst_pixels, uint32_t dst_stride,
    const float* __restrict src_pixels, uint32_t src_stride,
    int x0, int y0, int x1, int y1,
    int ox, int oy
) {
    const __m256 one = _mm256_set1_ps(1.0f);

    for (int y = y0; y < y1; ++y) {
        const float* src_row = src_pixels + ((size_t)y * src_stride + x0) * 4;
        float*       dst_row = dst_pixels + ((size_t)(y + oy) * dst_stride + (x0 + ox)) * 4;
        const int w = x1 - x0;
        int x = 0;

        // 2 pixels per iteration (8 floats, 1 __m256)
        for (; x + 1 < w; x += 2, src_row += 8, dst_row += 8) {
            __m256 s = _mm256_loadu_ps(src_row);
            __m256 d = _mm256_loadu_ps(dst_row);

            // Broadcast alpha of each pixel to all 4 channels in its lane.
            // _mm256_permute_ps with 0xFF = _MM_SHUFFLE(3,3,3,3): broadcasts element 3
            // within each 128-bit half independently.
            __m256 sa     = _mm256_permute_ps(s, 0xFF);
            __m256 inv_sa = _mm256_sub_ps(one, sa);

            // Premultiply: [r*a, g*a, b*a, a*a, ...]
            __m256 s_premul = _mm256_mul_ps(s, sa);
            // Fix alpha channels (bit positions 3 and 7): restore src.a instead of src.a^2.
            // 0x88 = 0b10001000: select from second operand at positions 3 and 7.
            s_premul = _mm256_blend_ps(s_premul, s, 0x88);

            // out = s_premul + dst * inv_sa
            __m256 result = _mm256_fmadd_ps(d, inv_sa, s_premul);
            _mm256_storeu_ps(dst_row, result);
        }

        // Scalar tail for odd-width tiles
        for (; x < w; ++x, src_row += 4, dst_row += 4) {
            const float sa = src_row[3];
            if (sa <= 0.0f) continue;
            const float inv_sa = 1.0f - sa;
            dst_row[0] = src_row[0] * sa + dst_row[0] * inv_sa;
            dst_row[1] = src_row[1] * sa + dst_row[1] * inv_sa;
            dst_row[2] = src_row[2] * sa + dst_row[2] * inv_sa;
            dst_row[3] = sa          +     dst_row[3] * inv_sa;
        }
    }
}

} // namespace tachyon::renderer2d
