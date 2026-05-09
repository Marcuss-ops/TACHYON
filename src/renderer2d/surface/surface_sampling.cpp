#include "tachyon/renderer2d/surface/surface_sampling.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

Color sample_texture_bilinear(const SurfaceRGBA& texture, float u, float v, Color tint) {
    const std::uint32_t width = texture.width();
    const std::uint32_t height = texture.height();
    if (width == 0 || height == 0) return Color::transparent();

    const float src_width = static_cast<float>(width);
    const float src_height = static_cast<float>(height);

    const float fx = std::clamp(u * src_width - 0.5f, 0.0f, src_width - 1.001f);
    const float fy = std::clamp(v * src_height - 0.5f, 0.0f, src_height - 1.001f);
    const std::uint32_t x0 = static_cast<std::uint32_t>(fx);
    const std::uint32_t y0 = static_cast<std::uint32_t>(fy);
    const float tx = fx - static_cast<float>(x0);
    const float ty = fy - static_cast<float>(y0);

    const auto& pixels = texture.pixels();
    const std::size_t stride = static_cast<std::size_t>(width) * 4;

    const auto get_raw_color = [&](std::uint32_t x, std::uint32_t y) -> Color {
        const std::size_t idx = (static_cast<std::size_t>(y) * stride) + (static_cast<std::size_t>(x) * 4);
        return { pixels[idx], pixels[idx + 1], pixels[idx + 2], pixels[idx + 3] };
    };

    // Fast path: near-integer coordinates
    if (tx < 0.001f && ty < 0.001f) {
        Color res = get_raw_color(x0, y0);
        return { res.r * tint.r, res.g * tint.g, res.b * tint.b, res.a * tint.a };
    }

    const std::uint32_t x1 = x0 + 1;
    const std::uint32_t y1 = y0 + 1;

    const Color p00 = get_raw_color(x0, y0);
    const Color p10 = get_raw_color(x1, y0);
    const Color p01 = get_raw_color(x0, y1);
    const Color p11 = get_raw_color(x1, y1);

    const float inv_tx = 1.0f - tx;
    const float inv_ty = 1.0f - ty;

    const float w00 = inv_tx * inv_ty;
    const float w10 = tx * inv_ty;
    const float w01 = inv_tx * ty;
    const float w11 = tx * ty;

    Color res = {
        p00.r * w00 + p10.r * w10 + p01.r * w01 + p11.r * w11,
        p00.g * w00 + p10.g * w10 + p01.g * w01 + p11.g * w11,
        p00.b * w00 + p10.b * w10 + p01.b * w01 + p11.b * w11,
        p00.a * w00 + p10.a * w10 + p01.a * w01 + p11.a * w11
    };

    return { res.r * tint.r, res.g * tint.g, res.b * tint.b, res.a * tint.a };
}

} // namespace tachyon::renderer2d
