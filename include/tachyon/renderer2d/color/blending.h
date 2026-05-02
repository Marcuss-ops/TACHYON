#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/color/transfer_functions.h"
#include "tachyon/renderer2d/color/color_matrix.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace tachyon::renderer2d {

/**
 * @brief Alpha representation mode.
 *
 * TACHYON uses premultiplied alpha internally for compositing
 * because it produces correct results under filtering, scaling,
 * and blending.  Straight alpha is supported at I/O boundaries.
 */
enum class AlphaMode {
    Straight,       ///< RGB and A stored separately (R,G,B not scaled by A)
    Premultiplied   ///< RGB already multiplied by A (standard for compositing)
};

enum class BlendMode {
    Normal,
    Multiply,
    Screen,
    Overlay,
    Darken,
    Lighten,
    ColorDodge,
    ColorBurn,
    HardLight,
    SoftLight,
    Difference,
    Exclusion,
    Hue,
    Saturation,
    Color,
    Luminosity,
    Additive,
    LinearDodge,
    Subtract,
    Divide,
    LinearBurn,
    VividLight,
    LinearLight,
    PinLight,
    HardMix,
    DarkerColor,
    LighterColor,
    StencilAlpha,
    StencilLuma,
    SilhouetteAlpha,
    SilhouetteLuma,
    AlphaAdd,
    LuminescentPremult
};

struct LinearPremultipliedPixel {
    float r{0.0f}, g{0.0f}, b{0.0f}, a{0.0f};
};

inline LinearPremultipliedPixel to_premultiplied(Color color, TransferCurve curve) {
    const float alpha = std::clamp(color.a, 0.0f, 1.0f);
    if (alpha <= 0.0f) return {};
    return {transfer_to_linear_component(color.r, curve) * alpha, transfer_to_linear_component(color.g, curve) * alpha, transfer_to_linear_component(color.b, curve) * alpha, alpha};
}

inline Color from_premultiplied(const LinearPremultipliedPixel& pixel, TransferCurve curve) {
    if (pixel.a <= 0.0f) return Color::transparent();
    const float inv_a = 1.0f / pixel.a;
    return Color{linear_to_transfer_component(pixel.r * inv_a, curve), linear_to_transfer_component(pixel.g * inv_a, curve), linear_to_transfer_component(pixel.b * inv_a, curve), pixel.a};
}

inline Color composite_src_over(Color src, Color dst, TransferCurve curve) {
    const auto s = to_premultiplied(src, curve);
    const auto d = to_premultiplied(dst, curve);
    const float inv_sa = 1.0f - s.a;
    return from_premultiplied({s.r + d.r * inv_sa, s.g + d.g * inv_sa, s.b + d.b * inv_sa, s.a + d.a * inv_sa}, curve);
}

inline Color composite_src_over_linear(Color src, Color dst) {
    return composite_src_over(src, dst, TransferCurve::Linear);
}

Color blend_mode_color_with_curve(Color src, Color dest, BlendMode mode, TransferCurve curve = TransferCurve::Linear);
Color blend_mode_color(Color src, Color dest, BlendMode mode);
BlendMode parse_blend_mode(const std::string& name);

inline Color convert_color(Color color, TransferCurve src_c, ColorSpace src_s, TransferCurve dst_c, ColorSpace dst_s) {
    if (src_c == dst_c && src_s == dst_s) return color;
    auto linear = to_premultiplied(color, src_c);
    if (src_s != dst_s && linear.a > 0.0f) {
        const float inv_a = 1.0f / linear.a;
        const auto matrix = primaries_conversion_matrix(static_cast<ColorPrimaries>(src_s), static_cast<ColorPrimaries>(dst_s));
        const float r = linear.r * inv_a, g = linear.g * inv_a, b = linear.b * inv_a;
        linear.r = std::max(0.0f, matrix.m[0] * r + matrix.m[1] * g + matrix.m[2] * b) * linear.a;
        linear.g = std::max(0.0f, matrix.m[3] * r + matrix.m[4] * g + matrix.m[5] * b) * linear.a;
        linear.b = std::max(0.0f, matrix.m[6] * r + matrix.m[7] * g + matrix.m[8] * b) * linear.a;
    }
    return from_premultiplied(linear, dst_c);
}

inline void apply_matte_buffer(SurfaceRGBA& surface, const std::vector<float>& matte, std::int64_t width, std::int64_t height) {
    if (matte.empty()) return;
    const std::size_t pixel_count = static_cast<std::size_t>(width * height);
    for (std::size_t i = 0; i < pixel_count && i < matte.size(); ++i) {
        const std::int64_t x = static_cast<std::int64_t>(i % static_cast<std::size_t>(width));
        const std::int64_t y = static_cast<std::int64_t>(i / static_cast<std::size_t>(width));
        if (x >= 0 && x < static_cast<std::int64_t>(surface.width()) && y >= 0 && y < static_cast<std::int64_t>(surface.height())) {
            auto px = surface.get_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y));
            px.a *= std::clamp(matte[i], 0.0f, 1.0f);
            surface.set_pixel(static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), px);
        }
    }
}

} // namespace tachyon::renderer2d
