#include "tachyon/renderer2d/color/blending.h"
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {

BlendMode parse_blend_mode(const std::string& name) {
    static const std::unordered_map<std::string, BlendMode> mode_map = {
        {"normal", BlendMode::Normal},
        {"multiply", BlendMode::Multiply},
        {"screen", BlendMode::Screen},
        {"overlay", BlendMode::Overlay},
        {"darken", BlendMode::Darken},
        {"lighten", BlendMode::Lighten},
        {"color_dodge", BlendMode::ColorDodge},
        {"color_burn", BlendMode::ColorBurn},
        {"hard_light", BlendMode::HardLight},
        {"soft_light", BlendMode::SoftLight},
        {"difference", BlendMode::Difference},
        {"exclusion", BlendMode::Exclusion},
        {"hue", BlendMode::Hue},
        {"saturation", BlendMode::Saturation},
        {"color", BlendMode::Color},
        {"luminosity", BlendMode::Luminosity},
        {"additive", BlendMode::Additive},
        {"linear_dodge", BlendMode::LinearDodge},
        {"subtract", BlendMode::Subtract},
        {"divide", BlendMode::Divide},
        {"linear_burn", BlendMode::LinearBurn},
        {"vivid_light", BlendMode::VividLight},
        {"linear_light", BlendMode::LinearLight},
        {"pin_light", BlendMode::PinLight},
        {"hard_mix", BlendMode::HardMix},
        {"darker_color", BlendMode::DarkerColor},
        {"lighter_color", BlendMode::LighterColor},
        {"stencil_alpha", BlendMode::StencilAlpha},
        {"stencil_luma", BlendMode::StencilLuma},
        {"silhouette_alpha", BlendMode::SilhouetteAlpha},
        {"silhouette_luma", BlendMode::SilhouetteLuma},
        {"alpha_add", BlendMode::AlphaAdd},
        {"luminescent_premult", BlendMode::LuminescentPremult}
    };

    auto it = mode_map.find(name);
    return it != mode_map.end() ? it->second : BlendMode::Normal;
}

Color blend_mode_color_with_curve(Color src, Color dest, BlendMode mode, TransferCurve curve) {
    if (mode == BlendMode::Normal) {
        return composite_src_over(src, dest, curve);
    }

    LinearPremultipliedPixel src_linear = to_premultiplied(src, curve);
    LinearPremultipliedPixel dst_linear = to_premultiplied(dest, curve);
    LinearPremultipliedPixel out;

    switch (mode) {
    case BlendMode::Normal:
        break; // Handled above
    case BlendMode::Additive:
        out.r = std::min(1.0f, src_linear.r + dst_linear.r);
        out.g = std::min(1.0f, src_linear.g + dst_linear.g);
        out.b = std::min(1.0f, src_linear.b + dst_linear.b);
        out.a = std::min(1.0f, src_linear.a + dst_linear.a);
        break;
    case BlendMode::Multiply:
        out.r = src_linear.r * dst_linear.r + src_linear.r * (1.0f - dst_linear.a) + dst_linear.r * (1.0f - src_linear.a);
        out.g = src_linear.g * dst_linear.g + src_linear.g * (1.0f - dst_linear.a) + dst_linear.g * (1.0f - src_linear.a);
        out.b = src_linear.b * dst_linear.b + src_linear.b * (1.0f - dst_linear.a) + dst_linear.b * (1.0f - src_linear.a);
        out.a = src_linear.a + dst_linear.a - src_linear.a * dst_linear.a;
        break;
    case BlendMode::Screen:
        out.r = src_linear.r + dst_linear.r - src_linear.r * dst_linear.r;
        out.g = src_linear.g + dst_linear.g - src_linear.g * dst_linear.g;
        out.b = src_linear.b + dst_linear.b - src_linear.b * dst_linear.b;
        out.a = src_linear.a + dst_linear.a - src_linear.a * dst_linear.a;
        break;
    case BlendMode::Overlay: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const auto overlay_channel = [](float src_channel, float dst_channel) {
            return dst_channel <= 0.5f ? 2.0f * src_channel * dst_channel : 1.0f - 2.0f * (1.0f - src_channel) * (1.0f - dst_channel);
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * overlay_channel(src_r, dst_r);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * overlay_channel(src_g, dst_g);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * overlay_channel(src_b, dst_b);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::SoftLight: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const auto soft_light_channel = [](float src_channel, float dst_channel) {
            if (src_channel <= 0.5f) return dst_channel - (1.0f - 2.0f * src_channel) * dst_channel * (1.0f - dst_channel);
            const float lifted = std::sqrt(std::clamp(dst_channel, 0.0f, 1.0f));
            return dst_channel + (2.0f * src_channel - 1.0f) * (lifted - dst_channel);
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * soft_light_channel(src_r, dst_r);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * soft_light_channel(src_g, dst_g);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * soft_light_channel(src_b, dst_b);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    default:
        return composite_src_over(src, dest, curve);
    }

    return from_premultiplied(out, curve);
}

Color blend_mode_color(Color src, Color dest, BlendMode mode) {
    return blend_mode_color_with_curve(src, dest, mode, TransferCurve::Linear);
}

} // namespace tachyon::renderer2d
