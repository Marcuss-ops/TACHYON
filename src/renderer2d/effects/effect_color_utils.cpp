#include "tachyon/renderer2d/effects/effect_common.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

LinearColor to_linear(Color color) {
    return LinearColor{
        detail::srgb_to_linear_component(color.r),
        detail::srgb_to_linear_component(color.g),
        detail::srgb_to_linear_component(color.b)
    };
}

Color from_linear(const LinearColor& color, float alpha) {
    return Color{
        static_cast<float>(detail::linear_to_srgb_component(color.r)) / 255.0f,
        static_cast<float>(detail::linear_to_srgb_component(color.g)) / 255.0f,
        static_cast<float>(detail::linear_to_srgb_component(color.b)) / 255.0f,
        alpha
    };
}

float luminance(const Color& color) {
    const LinearColor linear = to_linear(color);
    return clamp01(0.2126f * linear.r + 0.7152f * linear.g + 0.0722f * linear.b);
}

void rgb_to_hsl(float r, float g, float b, float& h, float& s, float& l) {
    const float max_v = std::max({r, g, b});
    const float min_v = std::min({r, g, b});
    l = (max_v + min_v) * 0.5f;

    if (max_v == min_v) {
        h = 0.0f;
        s = 0.0f;
        return;
    }

    const float d = max_v - min_v;
    s = l > 0.5f ? d / (2.0f - max_v - min_v) : d / (max_v + min_v);

    if (max_v == r) {
        h = (g - b) / d + (g < b ? 6.0f : 0.0f);
    } else if (max_v == g) {
        h = (b - r) / d + 2.0f;
    } else {
        h = (r - g) / d + 4.0f;
    }

    h /= 6.0f;
}

float hue_to_rgb(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
}

Color hsl_to_rgb(float h, float s, float l, float alpha) {
    h = std::fmod(h, 1.0f);
    if (h < 0.0f) h += 1.0f;
    s = clamp01(s);
    l = clamp01(l);

    if (s <= 0.0f) return Color{l, l, l, alpha};

    const float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
    const float p = 2.0f * l - q;
    const float r = hue_to_rgb(p, q, h + 1.0f / 3.0f);
    const float g = hue_to_rgb(p, q, h);
    const float b = hue_to_rgb(p, q, h - 1.0f / 3.0f);
    return Color{
        static_cast<float>(detail::linear_to_srgb_component(r)) / 255.0f,
        static_cast<float>(detail::linear_to_srgb_component(g)) / 255.0f,
        static_cast<float>(detail::linear_to_srgb_component(b)) / 255.0f,
        alpha
    };
}

PremultipliedPixel to_premultiplied(Color color) {
    const float alpha = color.a;
    return PremultipliedPixel{ color.r * alpha, color.g * alpha, color.b * alpha, color.a };
}

Color from_premultiplied(const PremultipliedPixel& px) {
    if (px.a <= 0.0f) return Color::transparent();
    const float inv = 1.0f / px.a;
    return Color{ std::clamp(px.r * inv, 0.0f, 1.0f), std::clamp(px.g * inv, 0.0f, 1.0f), std::clamp(px.b * inv, 0.0f, 1.0f), std::clamp(px.a, 0.0f, 1.0f) };
}

Color lerp_color(Color a, Color b, float t) {
    const float clamped_t = clamp01(t);
    return Color{
        lerp(a.r, b.r, clamped_t),
        lerp(a.g, b.g, clamped_t),
        lerp(a.b, b.b, clamped_t),
        lerp(a.a, b.a, clamped_t)
    };
}

} // namespace tachyon::renderer2d
