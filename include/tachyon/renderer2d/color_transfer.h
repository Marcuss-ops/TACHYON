#pragma once

#include "tachyon/renderer2d/framebuffer.h"

#include <array>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace tachyon::renderer2d {
namespace detail {

enum class ColorSpace {
    Srgb,
    Bt709,
    DciP3
};

enum class TransferCurve {
    Linear,
    Srgb,
    Bt709
};

inline std::string_view ascii_lower(std::string_view value) {
    return value;
}

inline ColorSpace parse_color_space(std::string_view value) {
    std::string lower;
    lower.reserve(value.size());
    for (unsigned char ch : value) {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }

    if (lower == "bt709" || lower == "rec709") {
        return ColorSpace::Bt709;
    }
    if (lower == "p3" || lower == "dci-p3" || lower == "display-p3") {
        return ColorSpace::DciP3;
    }
    return ColorSpace::Srgb;
}

inline TransferCurve parse_transfer_curve(std::string_view value) {
    std::string lower;
    lower.reserve(value.size());
    for (unsigned char ch : value) {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }

    if (lower == "linear" || lower == "linear_rec709" || lower == "rec709_linear") {
        return TransferCurve::Linear;
    }
    if (lower == "bt709" || lower == "rec709" || lower == "gamma_2.4") {
        return TransferCurve::Bt709;
    }
    return TransferCurve::Srgb;
}

inline const std::array<float, 256>& srgb_to_linear_lut() {
    static const std::array<float, 256> lut = []() {
        std::array<float, 256> values{};
        for (std::size_t index = 0; index < values.size(); ++index) {
            const float srgb = static_cast<float>(index) / 255.0f;
            values[index] = srgb <= 0.04045f
                ? srgb / 12.92f
                : std::pow((srgb + 0.055f) / 1.055f, 2.4f);
        }
        return values;
    }();
    return lut;
}

inline const std::array<std::uint8_t, 4096>& linear_to_srgb_lut() {
    static const std::array<std::uint8_t, 4096> lut = []() {
        std::array<std::uint8_t, 4096> values{};
        for (std::size_t index = 0; index < values.size(); ++index) {
            const float linear = static_cast<float>(index) / static_cast<float>(values.size() - 1U);
            float srgb;
            if (linear <= 0.0031308f) {
                srgb = linear * 12.92f;
            } else {
                srgb = 1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f;
            }
            values[index] = static_cast<std::uint8_t>(std::lround(std::clamp(srgb, 0.0f, 1.0f) * 255.0f));
        }
        return values;
    }();
    return lut;
}

inline std::uint8_t linear_to_srgb_component(float value) {
    const auto& lut = linear_to_srgb_lut();
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    const std::size_t index = static_cast<std::size_t>(std::lround(clamped * static_cast<float>(lut.size() - 1U)));
    return lut[index];
}

inline float srgb_to_linear_component(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped <= 0.04045f
        ? clamped / 12.92f
        : std::pow((clamped + 0.055f) / 1.055f, 2.4f);
}

inline float linear_to_srgb_component_float(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped <= 0.0031308f
        ? clamped * 12.92f
        : 1.055f * std::pow(clamped, 1.0f / 2.4f) - 0.055f;
}

inline float bt709_to_linear_component(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped < 0.081f
        ? clamped / 4.5f
        : std::pow((clamped + 0.099f) / 1.099f, 1.0f / 0.45f);
}

inline float linear_to_bt709_component(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped < 0.018f
        ? 4.5f * clamped
        : 1.099f * std::pow(clamped, 0.45f) - 0.099f;
}

inline float transfer_to_linear_component(float value, TransferCurve curve) {
    switch (curve) {
    case TransferCurve::Linear:
        return std::clamp(value, 0.0f, 1.0f);
    case TransferCurve::Bt709:
        return bt709_to_linear_component(value);
    case TransferCurve::Srgb:
    default:
        return srgb_to_linear_component(value);
    }
}

inline float linear_to_transfer_component(float value, TransferCurve curve) {
    switch (curve) {
    case TransferCurve::Linear:
        return std::clamp(value, 0.0f, 1.0f);
    case TransferCurve::Bt709:
        return linear_to_bt709_component(value);
    case TransferCurve::Srgb:
    default:
        return linear_to_srgb_component_float(value);
    }
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

inline LinearPremultipliedPixel to_premultiplied(Color color, TransferCurve transfer_curve);
inline Color from_premultiplied(const LinearPremultipliedPixel& pixel, TransferCurve transfer_curve);
inline Color composite_src_over(Color src, Color dst, TransferCurve transfer_curve);

inline LinearPremultipliedPixel to_linear_premultiplied(Color color) {
    return to_premultiplied(color, TransferCurve::Srgb);
}

inline LinearPremultipliedPixel to_premultiplied(Color color, TransferCurve transfer_curve) {
    const float alpha = static_cast<float>(color.a) / 255.0f;
    if (alpha <= 0.0f) {
        return {};
    }

    const float r = transfer_to_linear_component(static_cast<float>(color.r) / 255.0f, transfer_curve);
    const float g = transfer_to_linear_component(static_cast<float>(color.g) / 255.0f, transfer_curve);
    const float b = transfer_to_linear_component(static_cast<float>(color.b) / 255.0f, transfer_curve);
    return LinearPremultipliedPixel{
        r * alpha,
        g * alpha,
        b * alpha,
        alpha
    };
}

inline Color from_linear_premultiplied(const LinearPremultipliedPixel& pixel) {
    return from_premultiplied(pixel, TransferCurve::Srgb);
}

inline Color from_premultiplied(const LinearPremultipliedPixel& pixel, TransferCurve transfer_curve) {
    if (pixel.a <= 0.0f) {
        return Color::transparent();
    }

    const float inv_a = 1.0f / pixel.a;
    const float r = linear_to_transfer_component(pixel.r * inv_a, transfer_curve);
    const float g = linear_to_transfer_component(pixel.g * inv_a, transfer_curve);
    const float b = linear_to_transfer_component(pixel.b * inv_a, transfer_curve);
    return Color{
        static_cast<std::uint8_t>(std::clamp(std::lround(r * 255.0f), 0L, 255L)),
        static_cast<std::uint8_t>(std::clamp(std::lround(g * 255.0f), 0L, 255L)),
        static_cast<std::uint8_t>(std::clamp(std::lround(b * 255.0f), 0L, 255L)),
        static_cast<uint8_t>(std::clamp(std::lround(pixel.a * 255.0f), 0L, 255L))
    };
}

inline Color composite_src_over_linear(Color src, Color dst) {
    return composite_src_over(src, dst, TransferCurve::Srgb);
}

inline Color composite_src_over(Color src, Color dst, TransferCurve transfer_curve) {
    const LinearPremultipliedPixel src_linear = to_premultiplied(src, transfer_curve);
    const LinearPremultipliedPixel dst_linear = to_premultiplied(dst, transfer_curve);
    const float inv_src_a = 1.0f - src_linear.a;

    return from_premultiplied(LinearPremultipliedPixel{
        src_linear.r + dst_linear.r * inv_src_a,
        src_linear.g + dst_linear.g * inv_src_a,
        src_linear.b + dst_linear.b * inv_src_a,
        src_linear.a + dst_linear.a * inv_src_a
    }, transfer_curve);
}

inline Color convert_color(
    Color color,
    TransferCurve source_curve, ColorSpace source_space,
    TransferCurve dest_curve, ColorSpace dest_space) {

    if (source_curve == dest_curve && source_space == dest_space) {
        return color;
    }

    LinearPremultipliedPixel linear = to_premultiplied(color, source_curve);

    if (source_space != dest_space && linear.a > 0.0f) {
        const float inv_a = 1.0f / linear.a;
        float r = linear.r * inv_a;
        float g = linear.g * inv_a;
        float b = linear.b * inv_a;

        if ((source_space == ColorSpace::Srgb || source_space == ColorSpace::Bt709) && dest_space == ColorSpace::DciP3) {
            const float out_r = r * 0.8224621f + g * 0.177538f + b * 0.0f;
            const float out_g = r * 0.0331941f + g * 0.9668058f + b * 0.0f;
            const float out_b = r * 0.0170827f + g * 0.0723974f + b * 0.9105199f;
            r = out_r; g = out_g; b = out_b;
        } else if (source_space == ColorSpace::DciP3 && (dest_space == ColorSpace::Srgb || dest_space == ColorSpace::Bt709)) {
            const float out_r = r * 1.2249401f + g * -0.2249404f + b * 0.0f;
            const float out_g = r * -0.0420569f + g * 1.0420571f + b * 0.0f;
            const float out_b = r * -0.0196376f + g * -0.0786361f + b * 1.0982735f;
            r = out_r; g = out_g; b = out_b;
        }

        linear.r = std::max(0.0f, r) * linear.a;
        linear.g = std::max(0.0f, g) * linear.a;
        linear.b = std::max(0.0f, b) * linear.a;
    }

    return from_premultiplied(linear, dest_curve);
}

} // namespace detail

} // namespace tachyon::renderer2d
