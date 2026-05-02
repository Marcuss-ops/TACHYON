#include "tachyon/renderer2d/surface/surface_composite.h"
#include <algorithm>

namespace tachyon::renderer2d {

void composite_with_offset(SurfaceRGBA& dst, const SurfaceRGBA& src, int ox, int oy) {
    const int sw = static_cast<int>(src.width());
    const int sh = static_cast<int>(src.height());
    const int dw = static_cast<int>(dst.width());
    const int dh = static_cast<int>(dst.height());

    for (int y = 0; y < sh; ++y) {
        const int dy = y + oy;
        if (dy < 0 || dy >= dh) continue;

        for (int x = 0; x < sw; ++x) {
            const int dx = x + ox;
            if (dx < 0 || dx >= dw) continue;

            const Color src_pixel = src.get_pixel(x, y);
            if (src_pixel.a <= 0.0f) continue;

            if (src_pixel.a >= 1.0f) {
                dst.set_pixel(dx, dy, src_pixel);
            } else {
                Color dst_pixel = dst.get_pixel(dx, dy);
                const float alpha = src_pixel.a;
                const float inv_alpha = 1.0f - alpha;
                
                Color blended;
                blended.r = src_pixel.r * alpha + dst_pixel.r * inv_alpha;
                blended.g = src_pixel.g * alpha + dst_pixel.g * inv_alpha;
                blended.b = src_pixel.b * alpha + dst_pixel.b * inv_alpha;
                blended.a = std::max(src_pixel.a, dst_pixel.a);
                
                dst.set_pixel(dx, dy, blended);
            }
        }
    }
}

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
