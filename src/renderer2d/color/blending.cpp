#include "tachyon/renderer2d/color/blending.h"
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {


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
    case BlendMode::Darken: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        const float out_r = std::min(src_r, dst_r);
        const float out_g = std::min(src_g, dst_g);
        const float out_b = std::min(src_b, dst_b);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * out_r;
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * out_g;
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * out_b;
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::Lighten: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        const float out_r = std::max(src_r, dst_r);
        const float out_g = std::max(src_g, dst_g);
        const float out_b = std::max(src_b, dst_b);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * out_r;
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * out_g;
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * out_b;
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::ColorDodge: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const auto color_dodge = [](float src, float dst) {
            if (dst >= 1.0f) return 1.0f;
            if (src <= 0.0f) return 0.0f;
            return std::min(1.0f, dst / (1.0f - src));
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * color_dodge(src_r, dst_r);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * color_dodge(src_g, dst_g);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * color_dodge(src_b, dst_b);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::ColorBurn: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const auto color_burn = [](float src, float dst) {
            if (dst <= 0.0f) return 0.0f;
            if (src >= 1.0f) return 1.0f;
            return 1.0f - std::min(1.0f, (1.0f - dst) / src);
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * color_burn(src_r, dst_r);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * color_burn(src_g, dst_g);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * color_burn(src_b, dst_b);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::HardLight: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const auto hard_light = [](float src, float dst) {
            return src <= 0.5f ? 2.0f * src * dst : 1.0f - 2.0f * (1.0f - src) * (1.0f - dst);
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * hard_light(src_r, dst_r);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * hard_light(src_g, dst_g);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * hard_light(src_b, dst_b);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::Difference: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * std::abs(src_r - dst_r);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * std::abs(src_g - dst_g);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * std::abs(src_b - dst_b);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::Exclusion: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * (src_r + dst_r - 2.0f * src_r * dst_r);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * (src_g + dst_g - 2.0f * src_g * dst_g);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * (src_b + dst_b - 2.0f * src_b * dst_b);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::Hue: {
        const auto rgb_to_hsl = [](float r, float g, float b) {
            float max = std::max({r, g, b}), min = std::min({r, g, b});
            float h, s, l = (max + min) / 2.0f;
            float d = max - min;
            if (d == 0) h = 0;
            else if (max == r) h = std::fmod((g - b) / d + (g < b ? 6 : 0), 6.0f);
            else if (max == g) h = (b - r) / d + 2.0f;
            else h = (r - g) / d + 4.0f;
            h *= 60.0f;
            s = l > 0.5f ? d / (2.0f - max - min) : d / (max + min);
            return std::make_tuple(h, s, l);
        };
        const auto hsl_to_rgb = [](float h, float s, float l) {
            float c = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
            float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
            float m = l - c / 2.0f;
            float r, g, b;
            if (h < 60) { r = c; g = x; b = 0; }
            else if (h < 120) { r = x; g = c; b = 0; }
            else if (h < 180) { r = 0; g = c; b = x; }
            else if (h < 240) { r = 0; g = x; b = c; }
            else if (h < 300) { r = x; g = 0; b = c; }
            else { r = c; g = 0; b = x; }
            return std::make_tuple(r + m, g + m, b + m);
        };
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        auto [src_h, src_s, src_l] = rgb_to_hsl(src_r, src_g, src_b);
        auto [dst_h, dst_s, dst_l] = rgb_to_hsl(dst_r, dst_g, dst_b);
        auto [out_r_un, out_g_un, out_b_un] = hsl_to_rgb(src_h, dst_s, dst_l);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * std::clamp(out_r_un, 0.0f, 1.0f);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * std::clamp(out_g_un, 0.0f, 1.0f);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * std::clamp(out_b_un, 0.0f, 1.0f);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::Saturation: {
        const auto rgb_to_hsl = [](float r, float g, float b) {
            float max = std::max({r, g, b}), min = std::min({r, g, b});
            float h, s, l = (max + min) / 2.0f;
            float d = max - min;
            if (d == 0) h = 0;
            else if (max == r) h = std::fmod((g - b) / d + (g < b ? 6 : 0), 6.0f);
            else if (max == g) h = (b - r) / d + 2.0f;
            else h = (r - g) / d + 4.0f;
            h *= 60.0f;
            s = l > 0.5f ? d / (2.0f - max - min) : d / (max + min);
            return std::make_tuple(h, s, l);
        };
        const auto hsl_to_rgb = [](float h, float s, float l) {
            float c = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
            float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
            float m = l - c / 2.0f;
            float r, g, b;
            if (h < 60) { r = c; g = x; b = 0; }
            else if (h < 120) { r = x; g = c; b = 0; }
            else if (h < 180) { r = 0; g = c; b = x; }
            else if (h < 240) { r = 0; g = x; b = c; }
            else if (h < 300) { r = x; g = 0; b = c; }
            else { r = c; g = 0; b = x; }
            return std::make_tuple(r + m, g + m, b + m);
        };
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        auto [src_h, src_s, src_l] = rgb_to_hsl(src_r, src_g, src_b);
        auto [dst_h, dst_s, dst_l] = rgb_to_hsl(dst_r, dst_g, dst_b);
        auto [out_r_un, out_g_un, out_b_un] = hsl_to_rgb(dst_h, src_s, dst_l);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * std::clamp(out_r_un, 0.0f, 1.0f);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * std::clamp(out_g_un, 0.0f, 1.0f);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * std::clamp(out_b_un, 0.0f, 1.0f);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::Color: {
        const auto rgb_to_hsl = [](float r, float g, float b) {
            float max = std::max({r, g, b}), min = std::min({r, g, b});
            float h, s, l = (max + min) / 2.0f;
            float d = max - min;
            if (d == 0) h = 0;
            else if (max == r) h = std::fmod((g - b) / d + (g < b ? 6 : 0), 6.0f);
            else if (max == g) h = (b - r) / d + 2.0f;
            else h = (r - g) / d + 4.0f;
            h *= 60.0f;
            s = l > 0.5f ? d / (2.0f - max - min) : d / (max + min);
            return std::make_tuple(h, s, l);
        };
        const auto hsl_to_rgb = [](float h, float s, float l) {
            float c = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
            float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
            float m = l - c / 2.0f;
            float r, g, b;
            if (h < 60) { r = c; g = x; b = 0; }
            else if (h < 120) { r = x; g = c; b = 0; }
            else if (h < 180) { r = 0; g = c; b = x; }
            else if (h < 240) { r = 0; g = x; b = c; }
            else if (h < 300) { r = x; g = 0; b = c; }
            else { r = c; g = 0; b = x; }
            return std::make_tuple(r + m, g + m, b + m);
        };
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        auto [src_h, src_s, src_l] = rgb_to_hsl(src_r, src_g, src_b);
        auto [dst_h, dst_s, dst_l] = rgb_to_hsl(dst_r, dst_g, dst_b);
        auto [out_r_un, out_g_un, out_b_un] = hsl_to_rgb(src_h, src_s, dst_l);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * std::clamp(out_r_un, 0.0f, 1.0f);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * std::clamp(out_g_un, 0.0f, 1.0f);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * std::clamp(out_b_un, 0.0f, 1.0f);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::Luminosity: {
        const auto rgb_to_hsl = [](float r, float g, float b) {
            float max = std::max({r, g, b}), min = std::min({r, g, b});
            float h, s, l = (max + min) / 2.0f;
            float d = max - min;
            if (d == 0) h = 0;
            else if (max == r) h = std::fmod((g - b) / d + (g < b ? 6 : 0), 6.0f);
            else if (max == g) h = (b - r) / d + 2.0f;
            else h = (r - g) / d + 4.0f;
            h *= 60.0f;
            s = l > 0.5f ? d / (2.0f - max - min) : d / (max + min);
            return std::make_tuple(h, s, l);
        };
        const auto hsl_to_rgb = [](float h, float s, float l) {
            float c = (1.0f - std::abs(2.0f * l - 1.0f)) * s;
            float x = c * (1.0f - std::abs(std::fmod(h / 60.0f, 2.0f) - 1.0f));
            float m = l - c / 2.0f;
            float r, g, b;
            if (h < 60) { r = c; g = x; b = 0; }
            else if (h < 120) { r = x; g = c; b = 0; }
            else if (h < 180) { r = 0; g = c; b = x; }
            else if (h < 240) { r = 0; g = x; b = c; }
            else if (h < 300) { r = x; g = 0; b = c; }
            else { r = c; g = 0; b = x; }
            return std::make_tuple(r + m, g + m, b + m);
        };
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        auto [src_h, src_s, src_l] = rgb_to_hsl(src_r, src_g, src_b);
        auto [dst_h, dst_s, dst_l] = rgb_to_hsl(dst_r, dst_g, dst_b);
        auto [out_r_un, out_g_un, out_b_un] = hsl_to_rgb(dst_h, dst_s, src_l);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * std::clamp(out_r_un, 0.0f, 1.0f);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * std::clamp(out_g_un, 0.0f, 1.0f);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * std::clamp(out_b_un, 0.0f, 1.0f);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::LinearDodge: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * std::min(1.0f, src_r + dst_r);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * std::min(1.0f, src_g + dst_g);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * std::min(1.0f, src_b + dst_b);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::Subtract: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * std::max(0.0f, dst_r - src_r);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * std::max(0.0f, dst_g - src_g);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * std::max(0.0f, dst_b - src_b);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::Divide: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        const auto divide = [](float src, float dst) {
            return dst > 0.0f ? std::min(1.0f, src / dst) : 0.0f;
        };
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * divide(src_r, dst_r);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * divide(src_g, dst_g);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * divide(src_b, dst_b);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::LinearBurn: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * std::max(0.0f, src_r + dst_r - 1.0f);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * std::max(0.0f, src_g + dst_g - 1.0f);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * std::max(0.0f, src_b + dst_b - 1.0f);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::VividLight: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const auto vivid_light = [](float src, float dst) {
            if (src <= 0.5f) {
                if (dst <= 0.0f) return 0.0f;
                return 1.0f - (1.0f - dst) / (2.0f * src);
            } else {
                if (dst >= 1.0f) return 1.0f;
                return dst / (2.0f * (1.0f - src));
            }
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * std::clamp(vivid_light(src_r, dst_r), 0.0f, 1.0f);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * std::clamp(vivid_light(src_g, dst_g), 0.0f, 1.0f);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * std::clamp(vivid_light(src_b, dst_b), 0.0f, 1.0f);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::LinearLight: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * std::clamp(dst_r + 2.0f * src_r - 1.0f, 0.0f, 1.0f);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * std::clamp(dst_g + 2.0f * src_g - 1.0f, 0.0f, 1.0f);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * std::clamp(dst_b + 2.0f * src_b - 1.0f, 0.0f, 1.0f);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::PinLight: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        const auto pin_light = [](float src, float dst) {
            if (src <= 0.5f) return std::min(dst, 2.0f * src);
            else return std::max(dst, 2.0f * (src - 0.5f));
        };
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * std::clamp(pin_light(src_r, dst_r), 0.0f, 1.0f);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * std::clamp(pin_light(src_g, dst_g), 0.0f, 1.0f);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * std::clamp(pin_light(src_b, dst_b), 0.0f, 1.0f);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::HardMix: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        const auto hard_mix = [](float src, float dst) {
            return (src + dst) >= 1.0f ? 1.0f : 0.0f;
        };
        out.r = src_linear.r * (1.0f - dst_a) + dst_linear.r * (1.0f - src_a) + src_a * dst_a * hard_mix(src_r, dst_r);
        out.g = src_linear.g * (1.0f - dst_a) + dst_linear.g * (1.0f - src_a) + src_a * dst_a * hard_mix(src_g, dst_g);
        out.b = src_linear.b * (1.0f - dst_a) + dst_linear.b * (1.0f - src_a) + src_a * dst_a * hard_mix(src_b, dst_b);
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::DarkerColor: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        float src_sum = src_r + src_g + src_b;
        float dst_sum = dst_r + dst_g + dst_b;
        if (src_sum <= dst_sum) {
            out.r = src_linear.r; out.g = src_linear.g; out.b = src_linear.b;
        } else {
            out.r = dst_linear.r; out.g = dst_linear.g; out.b = dst_linear.b;
        }
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::LighterColor: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        const float dst_r = unpremultiply(dst_linear.r, dst_a), dst_g = unpremultiply(dst_linear.g, dst_a), dst_b = unpremultiply(dst_linear.b, dst_a);
        float src_sum = src_r + src_g + src_b;
        float dst_sum = dst_r + dst_g + dst_b;
        if (src_sum >= dst_sum) {
            out.r = src_linear.r; out.g = src_linear.g; out.b = src_linear.b;
        } else {
            out.r = dst_linear.r; out.g = dst_linear.g; out.b = dst_linear.b;
        }
        out.a = src_a + dst_a - src_a * dst_a;
        break;
    }
    case BlendMode::StencilAlpha: {
        out.r = dst_linear.r * (1.0f - src_linear.a);
        out.g = dst_linear.g * (1.0f - src_linear.a);
        out.b = dst_linear.b * (1.0f - src_linear.a);
        out.a = dst_linear.a * (1.0f - src_linear.a);
        break;
    }
    case BlendMode::StencilLuma: {
        const auto get_luma = [](float r, float g, float b) {
            return 0.299f * r + 0.587f * g + 0.114f * b;
        };
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        float src_luma = get_luma(src_r, src_g, src_b);
        out.r = dst_linear.r * (1.0f - src_luma);
        out.g = dst_linear.g * (1.0f - src_luma);
        out.b = dst_linear.b * (1.0f - src_luma);
        out.a = dst_linear.a * (1.0f - src_luma);
        break;
    }
    case BlendMode::SilhouetteAlpha: {
        out.r = dst_linear.r * src_linear.a;
        out.g = dst_linear.g * src_linear.a;
        out.b = dst_linear.b * src_linear.a;
        out.a = dst_linear.a * src_linear.a;
        break;
    }
    case BlendMode::SilhouetteLuma: {
        const auto get_luma = [](float r, float g, float b) {
            return 0.299f * r + 0.587f * g + 0.114f * b;
        };
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        float src_luma = get_luma(src_r, src_g, src_b);
        out.r = dst_linear.r * src_luma;
        out.g = dst_linear.g * src_luma;
        out.b = dst_linear.b * src_luma;
        out.a = dst_linear.a * src_luma;
        break;
    }
    case BlendMode::AlphaAdd: {
        out.r = src_linear.r + dst_linear.r;
        out.g = src_linear.g + dst_linear.g;
        out.b = src_linear.b + dst_linear.b;
        out.a = src_linear.a + dst_linear.a;
        break;
    }
    case BlendMode::LuminescentPremult: {
        const auto unpremultiply = [](float channel, float alpha) {
            return alpha > 0.0f ? std::clamp(channel / alpha, 0.0f, 1.0f) : 0.0f;
        };
        const float src_a = src_linear.a, dst_a = dst_linear.a;
        const float src_r = unpremultiply(src_linear.r, src_a), src_g = unpremultiply(src_linear.g, src_a), src_b = unpremultiply(src_linear.b, src_a);
        out.r = src_linear.r + dst_linear.r * (1.0f - src_r);
        out.g = src_linear.g + dst_linear.g * (1.0f - src_g);
        out.b = src_linear.b + dst_linear.b * (1.0f - src_b);
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
