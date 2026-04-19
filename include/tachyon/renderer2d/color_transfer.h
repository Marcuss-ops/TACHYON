#pragma once

#include "tachyon/renderer2d/framebuffer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace tachyon::renderer2d {
namespace detail {

inline float srgb_to_linear_component(float srgb) {
    if (srgb <= 0.04045f) {
        return srgb / 12.92f;
    }
    return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
}

inline std::uint8_t linear_to_srgb_component(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    float srgb;
    if (clamped <= 0.0031308f) {
        srgb = clamped * 12.92f;
    } else {
        srgb = 1.055f * std::pow(clamped, 1.0f / 2.4f) - 0.055f;
    }
    return static_cast<std::uint8_t>(std::lround(std::clamp(srgb, 0.0f, 1.0f) * 255.0f));
}

inline Color premultiply(Color color) {
    color.r = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.r) * color.a + 127U) / 255U);
    color.g = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.g) * color.a + 127U) / 255U);
    color.b = static_cast<std::uint8_t>((static_cast<std::uint32_t>(color.b) * color.a + 127U) / 255U);
    return color;
}

struct LinearPremultipliedPixel {
    float r{0.0f};
    float g{0.0f};
    float b{0.0f};
    float a{0.0f};
};

inline LinearPremultipliedPixel to_linear_premultiplied(Color color) {
    const float alpha = static_cast<float>(color.a) / 255.0f;
    if (alpha <= 0.0f) {
        return {};
    }

    const float straight_r = std::clamp(static_cast<float>(color.r) / 255.0f / alpha, 0.0f, 1.0f);
    const float straight_g = std::clamp(static_cast<float>(color.g) / 255.0f / alpha, 0.0f, 1.0f);
    const float straight_b = std::clamp(static_cast<float>(color.b) / 255.0f / alpha, 0.0f, 1.0f);
    return LinearPremultipliedPixel{
        srgb_to_linear_component(straight_r) * alpha,
        srgb_to_linear_component(straight_g) * alpha,
        srgb_to_linear_component(straight_b) * alpha,
        alpha
    };
}

inline Color from_linear_premultiplied(const LinearPremultipliedPixel& pixel) {
    if (pixel.a <= 0.0f) {
        return Color::transparent();
    }

    return Color{
        linear_to_srgb_component(pixel.r),
        linear_to_srgb_component(pixel.g),
        linear_to_srgb_component(pixel.b),
        static_cast<std::uint8_t>(std::clamp(std::lround(pixel.a * 255.0f), 0L, 255L))
    };
}

inline Color composite_src_over_linear(Color src, Color dst) {
    const LinearPremultipliedPixel src_linear = to_linear_premultiplied(src);
    const LinearPremultipliedPixel dst_linear = to_linear_premultiplied(dst);
    const float inv_src_a = 1.0f - src_linear.a;

    return from_linear_premultiplied(LinearPremultipliedPixel{
        src_linear.r + dst_linear.r * inv_src_a,
        src_linear.g + dst_linear.g * inv_src_a,
        src_linear.b + dst_linear.b * inv_src_a,
        src_linear.a + dst_linear.a * inv_src_a
    });
}

} // namespace detail

} // namespace tachyon::renderer2d
