#include "tachyon/renderer2d/effects/effect_common.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

Color sample_texture_bilinear(const SurfaceRGBA& texture, float u, float v, Color tint) {
    const float src_width = static_cast<float>(texture.width());
    const float src_height = static_cast<float>(texture.height());
    if (src_width <= 0.0F || src_height <= 0.0F) return Color::transparent();

    const float fx = std::clamp(u * src_width - 0.5f, 0.0f, src_width - 1.001f);
    const float fy = std::clamp(v * src_height - 0.5f, 0.0f, src_height - 1.001f);
    const std::uint32_t x0 = static_cast<std::uint32_t>(std::floor(fx));
    const std::uint32_t y0 = static_cast<std::uint32_t>(std::floor(fy));
    const std::uint32_t x1 = x0 + 1;
    const std::uint32_t y1 = y0 + 1;
    const float tx = fx - static_cast<float>(x0);
    const float ty = fy - static_cast<float>(y0);

    const Color p00 = texture.get_pixel(x0, y0);
    const Color p10 = texture.get_pixel(x1, y0);
    const Color p01 = texture.get_pixel(x0, y1);
    const Color p11 = texture.get_pixel(x1, y1);

    const auto lerp_px = [](Color a, Color b, float t) {
        return Color{ a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t };
    };

    const Color p0 = lerp_px(p00, p10, tx);
    const Color p1 = lerp_px(p01, p11, tx);
    Color res = lerp_px(p0, p1, ty);
    return Color{ res.r * tint.r, res.g * tint.g, res.b * tint.b, res.a * tint.a };
}

void composite_with_offset(SurfaceRGBA& dst, const SurfaceRGBA& src, int ox, int oy) {
    for (std::uint32_t y = 0; y < src.height(); ++y) {
        const int ty = static_cast<int>(y) + oy;
        if (ty < 0 || static_cast<std::uint32_t>(ty) >= dst.height()) continue;
        for (std::uint32_t x = 0; x < src.width(); ++x) {
            const int tx = static_cast<int>(x) + ox;
            if (tx < 0 || static_cast<std::uint32_t>(tx) >= dst.width()) continue;
            dst.blend_pixel(static_cast<std::uint32_t>(tx), static_cast<std::uint32_t>(ty), src.get_pixel(x, y));
        }
    }
}

} // namespace tachyon::renderer2d
