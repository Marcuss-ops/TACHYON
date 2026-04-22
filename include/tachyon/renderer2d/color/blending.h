#pragma once

#include "tachyon/renderer2d/color/transfer_functions.h"
#include "tachyon/renderer2d/color/color_matrix.h"

namespace tachyon::renderer2d {

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

} // namespace tachyon::renderer2d
