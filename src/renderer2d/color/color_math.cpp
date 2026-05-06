#include "tachyon/renderer2d/color/color_math.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/color/color_math.h"
#include <array>
#include <cmath>
#include <functional>

namespace tachyon::renderer2d {

float clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

float lerp(float a, float b, float t) {
    return a + (b - a) * t;
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

float luminance_rec709_linear(const Color& color) {
    const LinearColor linear = to_linear(color);
    return color::luminance_rec709(linear.r, linear.g, linear.b);
}

float luma_rec601(const Color& color) {
    return color::luma_rec601(color.r, color.g, color.b);
}

void rgb_to_hsl(float r, float g, float b, float& h, float& s, float& l) {
    color::rgb_to_hsl(r, g, b, h, s, l);
}

Color hsl_to_rgb(float h, float s, float l, float alpha) {
    auto rgb = color::hsl_to_rgb(h, s, l, alpha);
    return Color{
        static_cast<float>(detail::linear_to_srgb_component(rgb.r)) / 255.0f,
        static_cast<float>(detail::linear_to_srgb_component(rgb.g)) / 255.0f,
        static_cast<float>(detail::linear_to_srgb_component(rgb.b)) / 255.0f,
        alpha
    };
}

std::array<float, 256> build_channel_lut(std::function<float(float)> mapper) {
    std::array<float, 256> lut{};
    for (std::size_t i = 0; i < 256; ++i) {
        const float in = static_cast<float>(i) / 255.0f;
        lut[i] = std::clamp(mapper(in), 0.0f, 1.0f);
    }
    return lut;
}

SurfaceRGBA apply_channel_lut(const SurfaceRGBA& input, const std::array<float, 256>& lut) {
    return transform_surface(input, [&](Color px) {
        if (px.a == 0) return Color::transparent();
        const auto lookup = [&](float channel) -> float {
            const std::size_t index = static_cast<std::size_t>(std::lround(std::clamp(channel, 0.0f, 1.0f) * 255.0f));
            return lut[index];
        };
        return Color{lookup(px.r), lookup(px.g), lookup(px.b), px.a};
    });
}

} // namespace tachyon::renderer2d
