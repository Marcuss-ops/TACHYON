#include "tachyon/renderer2d/surface/surface_composite.h"
#include <algorithm>

#ifdef TACHYON_AVX2
namespace tachyon::renderer2d {
    extern void composite_tile_avx2(
        float* dst_pixels, uint32_t dst_stride,
        const float* src_pixels, uint32_t src_stride,
        int x0, int y0, int x1, int y1,
        int ox, int oy);
}
#endif

namespace tachyon::renderer2d {

// ---------------------------------------------------------------------------
// Scalar inner kernel — operates on raw float buffers, no virtual calls.
// ---------------------------------------------------------------------------
static void composite_tile_scalar(
    float* __restrict dst_pixels, uint32_t dst_stride,
    const float* __restrict src_pixels, uint32_t src_stride,
    int x0, int y0, int x1, int y1,
    int ox, int oy
) {
    for (int y = y0; y < y1; ++y) {
        const float* src_row = src_pixels + ((std::size_t)y * src_stride + x0) * 4;
        float*       dst_row = dst_pixels + ((std::size_t)(y + oy) * dst_stride + (x0 + ox)) * 4;
        const int w = x1 - x0;
        for (int x = 0; x < w; ++x, src_row += 4, dst_row += 4) {
            const float sa = src_row[3];
            if (sa <= 0.0f) continue;
            const float inv_sa = 1.0f - sa;
            dst_row[0] = src_row[0] * sa + dst_row[0] * inv_sa;
            dst_row[1] = src_row[1] * sa + dst_row[1] * inv_sa;
            dst_row[2] = src_row[2] * sa + dst_row[2] * inv_sa;
            dst_row[3] = sa          +     dst_row[3] * inv_sa; // Porter-Duff over
        }
    }
}

// ---------------------------------------------------------------------------
// Tiled composite — clips once, dispatches per tile.
// ---------------------------------------------------------------------------
void composite_with_offset_tiled(SurfaceRGBA& dst, const SurfaceRGBA& src,
                                  int ox, int oy, int tile_w, int tile_h) {
    const int x0 = std::max(0, -ox);
    const int y0 = std::max(0, -oy);
    const int x1 = std::min((int)src.width(),  (int)dst.width()  - ox);
    const int y1 = std::min((int)src.height(), (int)dst.height() - oy);
    if (x0 >= x1 || y0 >= y1) return;

    const float* sp = src.pixels().data();
    float*       dp = dst.mutable_pixels().data();
    const uint32_t sw = src.width();
    const uint32_t dw = dst.width();

    for (int ty = y0; ty < y1; ty += tile_h) {
        const int ty1 = std::min(ty + tile_h, y1);
        for (int tx = x0; tx < x1; tx += tile_w) {
            const int tx1 = std::min(tx + tile_w, x1);
#ifdef TACHYON_AVX2
            composite_tile_avx2(dp, dw, sp, sw, tx, ty, tx1, ty1, ox, oy);
#else
            composite_tile_scalar(dp, dw, sp, sw, tx, ty, tx1, ty1, ox, oy);
#endif
        }
    }
}

// ---------------------------------------------------------------------------
// Legacy entry point — delegates to tiled version.
// ---------------------------------------------------------------------------
void composite_with_offset(SurfaceRGBA& dst, const SurfaceRGBA& src, int ox, int oy) {
    composite_with_offset_tiled(dst, src, ox, oy);
}

// ---------------------------------------------------------------------------
// Premultiplied alpha helpers
// ---------------------------------------------------------------------------
PremultipliedPixel to_premultiplied(Color color) {
    return {
        color.r * color.a,
        color.g * color.a,
        color.b * color.a,
        color.a
    };
}

Color from_premultiplied(const PremultipliedPixel& px) {
    if (px.a <= 0.0f) return {0.0f, 0.0f, 0.0f, 0.0f};
    if (px.a >= 1.0f) return {px.r, px.g, px.b, 1.0f};
    const float inv_a = 1.0f / px.a;
    return {
        std::clamp(px.r * inv_a, 0.0f, 1.0f),
        std::clamp(px.g * inv_a, 0.0f, 1.0f),
        std::clamp(px.b * inv_a, 0.0f, 1.0f),
        px.a
    };
}

} // namespace tachyon::renderer2d
