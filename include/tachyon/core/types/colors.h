#pragma once

#include "tachyon/core/types/color_spec.h"
#include <algorithm>

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
