#include "tachyon/renderer2d/color/blending.h"
#include "tachyon/color/blending.h"
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {

namespace {
    inline color::LinearRGBA to_straight(const LinearPremultipliedPixel& p) {
        if (p.a <= 0.0f) return {0.0f, 0.0f, 0.0f, 0.0f};
        float inv_a = 1.0f / p.a;
        return {p.r * inv_a, p.g * inv_a, p.b * inv_a, p.a};
    }

    inline LinearPremultipliedPixel from_straight(const color::LinearRGBA& p) {
        return {p.r * p.a, p.g * p.a, p.b * p.a, p.a};
    }
}

Color blend_mode_color_with_curve(Color src, Color dest, BlendMode mode, TransferCurve curve) {
    if (mode == BlendMode::Normal) {
        return composite_src_over(src, dest, curve);
    }

    LinearPremultipliedPixel src_linear = to_premultiplied(src, curve);
    LinearPremultipliedPixel dst_linear = to_premultiplied(dest, curve);
    
    // Convert to straight alpha for canonical math
    color::LinearRGBA s = to_straight(src_linear);
    color::LinearRGBA d = to_straight(dst_linear);
    color::LinearRGBA out;

    switch (mode) {
    case BlendMode::Multiply:     out = color::blend_multiply(s, d); break;
    case BlendMode::Screen:       out = color::blend_screen(s, d); break;
    case BlendMode::Overlay:      out = color::blend_overlay(s, d); break;
    case BlendMode::Darken:       out = color::blend_darken(s, d); break;
    case BlendMode::Lighten:      out = color::blend_lighten(s, d); break;
    case BlendMode::ColorDodge:   out = color::blend_color_dodge(s, d); break;
    case BlendMode::ColorBurn:    out = color::blend_color_burn(s, d); break;
    case BlendMode::HardLight:    out = color::blend_hard_light(s, d); break;
    case BlendMode::SoftLight:    out = color::blend_soft_light(s, d); break;
    case BlendMode::Difference:   out = color::blend_difference(s, d); break;
    case BlendMode::Exclusion:    out = color::blend_exclusion(s, d); break;
    case BlendMode::Hue:          out = color::blend_hue(s, d); break;
    case BlendMode::Saturation:   out = color::blend_saturation(s, d); break;
    case BlendMode::Color:        out = color::blend_color(s, d); break;
    case BlendMode::Luminosity:   out = color::blend_luminosity(s, d); break;
    case BlendMode::Additive:
    case BlendMode::LinearDodge:  out = color::blend_linear_dodge(s, d); break;
    case BlendMode::Subtract:     out = color::blend_subtract(s, d); break;
    case BlendMode::Divide:       out = color::blend_divide(s, d); break;
    case BlendMode::LinearBurn:   out = color::blend_linear_burn(s, d); break;
    case BlendMode::VividLight:   out = color::blend_vivid_light(s, d); break;
    case BlendMode::LinearLight:  out = color::blend_linear_light(s, d); break;
    case BlendMode::PinLight:     out = color::blend_pin_light(s, d); break;
    case BlendMode::HardMix:      out = color::blend_hard_mix(s, d); break;
    case BlendMode::DarkerColor:  out = color::blend_darker_color(s, d); break;
    case BlendMode::LighterColor: out = color::blend_lighter_color(s, d); break;
    
    // These remain renderer-specific or use different logic
    case BlendMode::StencilAlpha:
        return from_premultiplied({dst_linear.r * (1.0f - src_linear.a), dst_linear.g * (1.0f - src_linear.a), dst_linear.b * (1.0f - src_linear.a), dst_linear.a * (1.0f - src_linear.a)}, curve);
    case BlendMode::StencilLuma: {
        float luma = 0.299f * s.r + 0.587f * s.g + 0.114f * s.b;
        return from_premultiplied({dst_linear.r * (1.0f - luma), dst_linear.g * (1.0f - luma), dst_linear.b * (1.0f - luma), dst_linear.a * (1.0f - luma)}, curve);
    }
    case BlendMode::SilhouetteAlpha:
        return from_premultiplied({dst_linear.r * src_linear.a, dst_linear.g * src_linear.a, dst_linear.b * src_linear.a, dst_linear.a * src_linear.a}, curve);
    case BlendMode::SilhouetteLuma: {
        float luma = 0.299f * s.r + 0.587f * s.g + 0.114f * s.b;
        return from_premultiplied({dst_linear.r * luma, dst_linear.g * luma, dst_linear.b * luma, dst_linear.a * luma}, curve);
    }
    case BlendMode::AlphaAdd:
        return from_premultiplied({src_linear.r + dst_linear.r, src_linear.g + dst_linear.g, src_linear.b + dst_linear.b, src_linear.a + dst_linear.a}, curve);
    case BlendMode::LuminescentPremult:
        return from_premultiplied({src_linear.r + dst_linear.r * (1.0f - s.r), src_linear.g + dst_linear.g * (1.0f - s.g), src_linear.b + dst_linear.b * (1.0f - s.b), src_linear.a + dst_linear.a - src_linear.a * dst_linear.a}, curve);

    default:
        return composite_src_over(src, dest, curve);
    }

    return from_premultiplied(from_straight(out), curve);
}

Color blend_mode_color(Color src, Color dest, BlendMode mode) {
    return blend_mode_color_with_curve(src, dest, mode, TransferCurve::Linear);
}

} // namespace tachyon::renderer2d
