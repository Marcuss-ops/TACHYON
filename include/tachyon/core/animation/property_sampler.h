#pragma once

#include "tachyon/core/animation/animated_property.h"
#include <algorithm>
#include <functional>

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

} // namespace tachyon::animation
