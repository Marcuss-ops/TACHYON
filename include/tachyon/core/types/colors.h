#pragma once

#include <cstdint>
#include <algorithm>

namespace tachyon {

/**
 * @brief Simple 8-bit RGBA color specification used in project schemas.
 */
struct ColorSpec {
    std::uint8_t r{255}, g{255}, b{255}, a{255};

    [[nodiscard]] bool operator==(const ColorSpec& other) const {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    [[nodiscard]] bool operator!=(const ColorSpec& other) const {
        return !(*this == other);
    }
};

} // namespace tachyon

#include "tachyon/core/animation/keyframe.h"
#include "tachyon/core/animation/animation_curve.h"

namespace tachyon {
namespace animation {

/**
 * LerpTraits specialization for ColorSpec.
 * Linearly interpolates each color channel independently.
 */
template <>
struct LerpTraits<ColorSpec> {
    static ColorSpec lerp(const ColorSpec& a, const ColorSpec& b, double t) {
        const float ft = static_cast<float>(t);
        return {
            static_cast<std::uint8_t>(std::clamp(a.r + (b.r - a.r) * ft, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(a.g + (b.g - a.g) * ft, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(a.b + (b.b - a.b) * ft, 0.0f, 255.0f)),
            static_cast<std::uint8_t>(std::clamp(a.a + (b.a - a.a) * ft, 0.0f, 255.0f))
        };
    }
};

template <>
inline ColorSpec hermite_interp<ColorSpec>(const Keyframe<ColorSpec>& k0, const Keyframe<ColorSpec>& k1, double t) {
    return LerpTraits<ColorSpec>::lerp(k0.value, k1.value, t);
}

} // namespace animation
} // namespace tachyon
