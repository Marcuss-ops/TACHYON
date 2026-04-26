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

Color convert_color(Color color, TransferCurve src_c, ColorSpace src_s, TransferCurve dst_c, ColorSpace dst_s);

void apply_matte_buffer(SurfaceRGBA& surface, const std::vector<float>& matte, std::int64_t width, std::int64_t height);

BlendMode parse_blend_mode(const std::string& name);

} // namespace tachyon::renderer2d
