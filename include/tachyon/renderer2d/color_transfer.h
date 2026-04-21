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
    sRGB,
    Linear,
    Rec709,
    DisplayP3
};

using ColorPrimaries = ColorSpace;

enum class ColorRange {
    Full,
    Limited
};

enum class TransferCurve {
    Linear,
    sRGB,
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
        return ColorSpace::Rec709;
    }
    if (lower == "p3" || lower == "dci-p3" || lower == "display-p3") {
        return ColorSpace::DisplayP3;
    }
    if (lower == "linear") {
        return ColorSpace::Linear;
    }
    return ColorSpace::sRGB;
}

inline ColorPrimaries parse_color_primaries(std::string_view value) {
    std::string lower;
    lower.reserve(value.size());
    for (unsigned char ch : value) {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }

    if (lower == "bt709" || lower == "rec709") {
        return ColorPrimaries::Rec709;
    }
    if (lower == "p3" || lower == "dci-p3" || lower == "display-p3") {
        return ColorPrimaries::DisplayP3;
    }
    if (lower == "linear") {
        return ColorPrimaries::Linear;
    }
    return ColorPrimaries::sRGB;
}

inline ColorRange parse_color_range(std::string_view value) {
    std::string lower;
    lower.reserve(value.size());
    for (unsigned char ch : value) {
        lower.push_back(static_cast<char>(std::tolower(ch)));
    }

    if (lower == "tv" || lower == "limited" || lower == "legal") {
        return ColorRange::Limited;
    }
    return ColorRange::Full;
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
    return TransferCurve::sRGB;
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

// User-facing Phase 1 helpers
inline float sRGB_to_Linear_f(float value) { return srgb_to_linear_component(value); }
inline float Linear_to_sRGB_f(float value) { return linear_to_srgb_component_float(value); }

inline Color sRGB_to_Linear(Color srgb) {
    return Color{
        sRGB_to_Linear_f(srgb.r),
        sRGB_to_Linear_f(srgb.g),
        sRGB_to_Linear_f(srgb.b),
        srgb.a
    };
}

inline Color Linear_to_sRGB(Color linear) {
    return Color{
        Linear_to_sRGB_f(linear.r),
        Linear_to_sRGB_f(linear.g),
        Linear_to_sRGB_f(linear.b),
        linear.a
    };
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
    const float normalized = std::clamp(value, 0.0f, 1.0f);
    switch (curve) {
    case TransferCurve::Linear:
        return normalized;
    case TransferCurve::Bt709:
        return bt709_to_linear_component(normalized);
    case TransferCurve::sRGB:
    default:
        return srgb_to_linear_component(normalized);
    }
}

inline float linear_to_transfer_component(float value, TransferCurve curve) {
    const float normalized = std::clamp(value, 0.0f, 1.0f);
    switch (curve) {
    case TransferCurve::Linear:
        return normalized;
    case TransferCurve::Bt709:
        return linear_to_bt709_component(normalized);
    case TransferCurve::sRGB:
    default:
        return linear_to_srgb_component_float(normalized);
    }
}

inline Color premultiply(Color color) {
    const float alpha = std::clamp(color.a, 0.0f, 1.0f);
    color.r *= alpha;
    color.g *= alpha;
    color.b *= alpha;
    color.a = alpha;
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
    return to_premultiplied(color, TransferCurve::Linear);
}

inline LinearPremultipliedPixel to_premultiplied(Color color, TransferCurve transfer_curve) {
    const float alpha = std::clamp(color.a, 0.0f, 1.0f);
    if (alpha <= 0.0f) {
        return {};
    }

    const float r = transfer_to_linear_component(color.r, transfer_curve);
    const float g = transfer_to_linear_component(color.g, transfer_curve);
    const float b = transfer_to_linear_component(color.b, transfer_curve);
    return LinearPremultipliedPixel{
        r * alpha,
        g * alpha,
        b * alpha,
        alpha
    };
}

inline Color from_linear_premultiplied(const LinearPremultipliedPixel& pixel) {
    return from_premultiplied(pixel, TransferCurve::Linear);
}

inline Color from_premultiplied(const LinearPremultipliedPixel& pixel, TransferCurve transfer_curve) {
    if (pixel.a <= 0.0f) {
        return Color::transparent();
    }

    const float inv_a = 1.0f / pixel.a;
    const float r = linear_to_transfer_component(pixel.r * inv_a, transfer_curve);
    const float g = linear_to_transfer_component(pixel.g * inv_a, transfer_curve);
    const float b = linear_to_transfer_component(pixel.b * inv_a, transfer_curve);
    return Color{ r, g, b, pixel.a };
}

inline Color composite_src_over_linear(Color src, Color dst) {
    return composite_src_over(src, dst, TransferCurve::Linear);
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

struct Matrix3x3 {
    float m[9]{};

    static const Matrix3x3& identity() {
        static const Matrix3x3 id{{1, 0, 0, 0, 1, 0, 0, 0, 1}};
        return id;
    }

    static const Matrix3x3& rec709_to_p3() {
        static const Matrix3x3 m{{
            0.8224621f, 0.177538f, 0.0f,
            0.0331941f, 0.9668058f, 0.0f,
            0.0170827f, 0.0723974f, 0.9105199f
        }};
        return m;
    }

    static const Matrix3x3& p3_to_rec709() {
        static const Matrix3x3 m{{
            1.2249401f, -0.2249404f, 0.0f,
            -0.0420569f, 1.0420571f, 0.0f,
            -0.0196376f, -0.0786361f, 1.0982735f
        }};
        return m;
    }
};

inline Matrix3x3 multiply(const Matrix3x3& a, const Matrix3x3& b) {
    Matrix3x3 result;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            result.m[row * 3 + col] =
                a.m[row * 3 + 0] * b.m[0 * 3 + col] +
                a.m[row * 3 + 1] * b.m[1 * 3 + col] +
                a.m[row * 3 + 2] * b.m[2 * 3 + col];
        }
    }
    return result;
}

inline Matrix3x3 invert(const Matrix3x3& matrix) {
    const float a = matrix.m[0], b = matrix.m[1], c = matrix.m[2];
    const float d = matrix.m[3], e = matrix.m[4], f = matrix.m[5];
    const float g = matrix.m[6], h = matrix.m[7], i = matrix.m[8];

    const float A =   e * i - f * h;
    const float B = -(d * i - f * g);
    const float C =   d * h - e * g;
    const float D = -(b * i - c * h);
    const float E =   a * i - c * g;
    const float F = -(a * h - b * g);
    const float G =   b * f - c * e;
    const float H = -(a * f - c * d);
    const float I =   a * e - b * d;

    const float det = a * A + b * B + c * C;
    if (std::abs(det) <= 1.0e-8f) {
        return Matrix3x3{};
    }

    const float inv_det = 1.0f / det;
    return Matrix3x3{{
        A * inv_det, D * inv_det, G * inv_det,
        B * inv_det, E * inv_det, H * inv_det,
        C * inv_det, F * inv_det, I * inv_det
    }};
}

inline Matrix3x3 rgb_to_xyz_matrix(ColorPrimaries primaries) {
    switch (primaries) {
    case ColorPrimaries::Rec709:
    case ColorPrimaries::sRGB:
        return Matrix3x3{{
            0.41239079926595934f, 0.3575843393838780f, 0.1804807884018343f,
            0.21263900587151027f, 0.7151686787677560f, 0.0721923153607337f,
            0.01933081871559182f, 0.1191947797946260f, 0.9505321522496607f
        }};
    case ColorPrimaries::DisplayP3:
        return Matrix3x3{{
            0.4865709486482162f, 0.2656676931690931f, 0.1982172852343625f,
            0.2289745640697488f, 0.6917385218365064f, 0.0792869140937450f,
            0.0000000000000000f, 0.0451133818589026f, 1.0439443689009760f
        }};
    case ColorPrimaries::Linear: // Assume sRGB primaries for now
        return Matrix3x3{{
            0.41239079926595934f, 0.3575843393838780f, 0.1804807884018343f,
            0.21263900587151027f, 0.7151686787677560f, 0.0721923153607337f,
            0.01933081871559182f, 0.1191947797946260f, 0.9505321522496607f
        }};
    }
    return Matrix3x3::identity();
}

inline Matrix3x3 primaries_conversion_matrix(ColorPrimaries source, ColorPrimaries destination) {
    return multiply(invert(rgb_to_xyz_matrix(destination)), rgb_to_xyz_matrix(source));
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
        const float r = linear.r * inv_a;
        const float g = linear.g * inv_a;
        const float b = linear.b * inv_a;
        const Matrix3x3 matrix = primaries_conversion_matrix(static_cast<ColorPrimaries>(source_space), static_cast<ColorPrimaries>(dest_space));
        const float out_r = matrix.m[0] * r + matrix.m[1] * g + matrix.m[2] * b;
        const float out_g = matrix.m[3] * r + matrix.m[4] * g + matrix.m[5] * b;
        const float out_b = matrix.m[6] * r + matrix.m[7] * g + matrix.m[8] * b;
        linear.r = std::max(0.0f, out_r) * linear.a;
        linear.g = std::max(0.0f, out_g) * linear.a;
        linear.b = std::max(0.0f, out_b) * linear.a;
    }

    return from_premultiplied(linear, dest_curve);
}

inline Color apply_primaries_matrix(Color color, const Matrix3x3& matrix) {
    const float r = color.r;
    const float g = color.g;
    const float b = color.b;
    const float out_r = matrix.m[0] * r + matrix.m[1] * g + matrix.m[2] * b;
    const float out_g = matrix.m[3] * r + matrix.m[4] * g + matrix.m[5] * b;
    const float out_b = matrix.m[6] * r + matrix.m[7] * g + matrix.m[8] * b;
    return Color{out_r, out_g, out_b, color.a};
}

inline Color apply_range_mode(Color color, ColorRange range) {
    if (range == ColorRange::Full) {
        return color;
    }

    auto scale = [](float value) -> float {
        return (16.0f + value * 219.0f) / 255.0f;
    };

    return Color{scale(color.r), scale(color.g), scale(color.b), color.a};
}

} // namespace detail

} // namespace tachyon::renderer2d
