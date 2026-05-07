#pragma once

#include "tachyon/core/animation/animated_property.h"
#include "tachyon/core/spec/schema/properties/property_spec.h"
#include "tachyon/core/types/color_spec.h"
#include "tachyon/core/math/vector2.h"
#include <algorithm>
#include <functional>
#include <optional>

namespace tachyon::animation {

/**
 * @brief Generic function to sample a value from a collection of keyframes at a specific time.
 * 
 * @tparam T The type of the value to sample.
 * @tparam LerpFn A function/lambda with signature T(const T& a, const T& b, double t).
 * 
 * @param keyframes Sorted list of keyframes.
 * @param fallback Value to return if no keyframes are available.
 * @param time The time in seconds to sample at.
 * @param lerp The interpolation function to use.
 * @return The sampled value.
 */
template <typename T, typename LerpFn>
T sample_keyframes(
    const std::vector<KeyframeSpec<T>>& keyframes,
    const T& fallback,
    double time,
    LerpFn lerp
) {
    if (keyframes.empty()) {
        return fallback;
    }

    // 1. Clamp to boundaries
    if (time <= keyframes.front().time) {
        return keyframes.front().value;
    }
    if (time >= keyframes.back().time) {
        return keyframes.back().value;
    }

    // 2. Find the interval [it - 1, it]
    auto it = std::lower_bound(keyframes.begin(), keyframes.end(), time,
        [](const KeyframeSpec<T>& k, double t) {
            return k.time < t;
        });

    const auto& k1 = *(it - 1);
    const auto& k2 = *it;

    // 3. Handle 'Hold' interpolation immediately
    if (k1.timing.interpolation == InterpolationMode::Hold) {
        return k1.value;
    }

    // 4. Calculate normalized t [0, 1]
    double duration = k2.time - k1.time;
    double t = (duration > 1e-8) ? (time - k1.time) / duration : 0.0;

    // 5. Apply easing remapping
    double remapped_t = apply_easing(t, k1.timing.easing, k1.timing.bezier);

    // 6. Interpolate
    return lerp(k1.value, k2.value, remapped_t);
}

/**
 * @brief Generic function to sample a value from a collection of spatial keyframes (with tangents).
 */
template <typename T, typename LerpFn, typename BezierFn>
T sample_spatial_keyframes(
    const std::vector<SpatialKeyframeSpec<T>>& keyframes,
    const T& fallback,
    double time,
    LerpFn lerp,
    BezierFn bezier_sample
) {
    if (keyframes.empty()) {
        return fallback;
    }

    if (time <= keyframes.front().time) {
        return keyframes.front().value;
    }
    if (time >= keyframes.back().time) {
        return keyframes.back().value;
    }

    auto it = std::lower_bound(keyframes.begin(), keyframes.end(), time,
        [](const SpatialKeyframeSpec<T>& k, double t) {
            return k.time < t;
        });

    const auto& k1 = *(it - 1);
    const auto& k2 = *it;

    if (k1.timing.interpolation == InterpolationMode::Hold) {
        return k1.value;
    }

    double duration = k2.time - k1.time;
    double t = (duration > 1e-8) ? (time - k1.time) / duration : 0.0;
    double remapped_t = apply_easing(t, k1.timing.easing, k1.timing.bezier);
    float weight = static_cast<float>(remapped_t);

    // If spatial tangents are present, use Bezier sampling
    // Note: We use a small epsilon to avoid unnecessary cubic sampling for zero-length tangents
    constexpr float kTangentEpsilon = 1.0e-6f;
    if (k1.tangent_out.length_squared() > kTangentEpsilon || k2.tangent_in.length_squared() > kTangentEpsilon) {
        return bezier_sample(
            k1.value,
            k1.value + k1.tangent_out,
            k2.value + k2.tangent_in,
            k2.value,
            weight
        );
    }

    return lerp(k1.value, k2.value, weight);
}

double sample_scalar(
    const std::optional<double>& static_value,
    const std::vector<ScalarKeyframeSpec>& keyframes,
    double time,
    double fallback = 0.0
);

math::Vector2 sample_vector2(
    const std::optional<math::Vector2>& static_value,
    const std::vector<Vector2KeyframeSpec>& keyframes,
    double time,
    math::Vector2 fallback = math::Vector2{0.0f, 0.0f}
);

ColorSpec sample_color(
    const std::optional<ColorSpec>& static_value,
    const std::vector<ColorKeyframeSpec>& keyframes,
    double time,
    ColorSpec fallback = ColorSpec{255, 255, 255, 255}
);

} // namespace tachyon::animation
