#pragma once

#include "tachyon/renderer2d/color/color_space.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <array>
#include <cmath>
#include <algorithm>

namespace tachyon::renderer2d {

inline float srgb_to_linear_component(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped <= 0.04045f ? clamped / 12.92f : std::pow((clamped + 0.055f) / 1.055f, 2.4f);
}

inline float linear_to_srgb_component_float(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped <= 0.0031308f ? clamped * 12.92f : 1.055f * std::pow(clamped, 1.0f / 2.4f) - 0.055f;
}

inline float bt709_to_linear_component(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped < 0.081f ? clamped / 4.5f : std::pow((clamped + 0.099f) / 1.099f, 1.0f / 0.45f);
}

inline float linear_to_bt709_component(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return clamped < 0.018f ? 4.5f * clamped : 1.099f * std::pow(clamped, 0.45f) - 0.099f;
}

inline float transfer_to_linear_component(float value, TransferCurve curve) {
    const float normalized = std::clamp(value, 0.0f, 1.0f);
    switch (curve) {
        case TransferCurve::Linear: return normalized;
        case TransferCurve::Bt709: return bt709_to_linear_component(normalized);
        case TransferCurve::sRGB:
        default: return srgb_to_linear_component(normalized);
    }
}

inline float linear_to_transfer_component(float value, TransferCurve curve) {
    const float normalized = std::clamp(value, 0.0f, 1.0f);
    switch (curve) {
        case TransferCurve::Linear: return normalized;
        case TransferCurve::Bt709: return linear_to_bt709_component(normalized);
        case TransferCurve::sRGB:
        default: return linear_to_srgb_component_float(normalized);
    }
}

inline Color sRGB_to_Linear(Color srgb) {
    return Color{srgb_to_linear_component(srgb.r), srgb_to_linear_component(srgb.g), srgb_to_linear_component(srgb.b), srgb.a};
}

inline Color Linear_to_sRGB(Color linear) {
    return Color{linear_to_srgb_component_float(linear.r), linear_to_srgb_component_float(linear.g), linear_to_srgb_component_float(linear.b), linear.a};
}

// Compatibility aliases
inline float sRGB_to_Linear_f(float value) { return srgb_to_linear_component(value); }
inline float Linear_to_sRGB_f(float value) { return linear_to_srgb_component_float(value); }

inline std::uint8_t linear_to_srgb_component(float value) {
    return static_cast<std::uint8_t>(std::lround(std::clamp(linear_to_srgb_component_float(value), 0.0f, 1.0f) * 255.0f));
}

} // namespace tachyon::renderer2d
