#pragma once

#include "tachyon/core/animation/keyframe.h"

#include <algorithm>
#include <cassert>
#include <vector>

namespace tachyon {
namespace animation {

/**
 * Lerp trait: provides a generic lerp(a, b, t) implementation.
 * Specialisations can be added for Quaternion (slerp), Color, etc.
 */
template <typename T>
struct LerpTraits {
    /**
     * Linearly interpolates from a to b by t.
     * Requires that T supports:
     *   T operator*(float) const;
     *   T operator+(const T&) const;
     */
    [[nodiscard]] static T lerp(const T& a, const T& b, double t) {
        const auto ft = static_cast<float>(t);
        return a * (1.0f - ft) + b * ft;
    }
};

// Explicit full specialisation for double (avoids float cast)
template <>
struct LerpTraits<double> {
    [[nodiscard]] static double lerp(double a, double b, double t) {
        return a + (b - a) * t;
    }
};

/**
 * Hermite cubic interpolation between two keyframes.
 * Uses the tangent handles stored in the keyframes.
 */
template <typename T>
[[nodiscard]] T hermite_interp(const Keyframe<T>& k0, const Keyframe<T>& k1, double t) {
    // t in [0,1]
    const double t2 = t * t;
    const double t3 = t2 * t;
    // Hermite basis
    const double h00 =  2.0 * t3 - 3.0 * t2 + 1.0;  // blend k0 value
    const double h10 =        t3 - 2.0 * t2 + t;     // blend k0 out tangent
    const double h01 = -2.0 * t3 + 3.0 * t2;         // blend k1 value
    const double h11 =        t3 -        t2;         // blend k1 in tangent

    const double seg_dt = k1.time - k0.time;

    // Tangent values scaled to segment length
    const T m0 = k0.out_tangent_value * static_cast<float>(seg_dt > 0 ? 1.0 / seg_dt : 0.0);
    const T m1 = k1.in_tangent_value  * static_cast<float>(seg_dt > 0 ? 1.0 / seg_dt : 0.0);

    return k0.value   * static_cast<float>(h00)
         + m0         * static_cast<float>(h10)
         + k1.value   * static_cast<float>(h01)
         + m1         * static_cast<float>(h11);
}

/**
 * AnimationCurve<T> — a sorted sequence of keyframes that can be evaluated
 * at any time t, returning an interpolated value of type T.
 *
 * Contract:
 *  - Keyframes must be added in ascending time order (or call sort()).
 *  - Evaluation is deterministic: same t → same result, regardless of call order.
 *  - Out-of-range evaluation clamps to the first/last keyframe value.
 *
 * @tparam T  Value type. Must satisfy LerpTraits<T>.
 */
template <typename T>
class AnimationCurve {
public:
    using KeyframeT = Keyframe<T>;

    /** Legacy public field kept for backward compat with existing property_tests.cpp. */
    std::vector<KeyframeT> keyframes;

    // --- Construction helpers ---------------------------------------------------

    /** Adds a keyframe. Caller must maintain ascending time order or call sort(). */
    void add_keyframe(const KeyframeT& kf) {
        keyframes.push_back(kf);
    }

    /** Convenience: add a linear keyframe at (time, value). */
    void add_linear(double time, const T& value) {
        KeyframeT kf;
        kf.time = time;
        kf.value = value;
        kf.out_mode = InterpolationMode::Linear;
        kf.in_mode  = InterpolationMode::Linear;
        kf.easing   = EasingPreset::None;
        keyframes.push_back(kf);
    }

    /** Convenience: add a Hold keyframe. */
    void add_hold(double time, const T& value) {
        KeyframeT kf;
        kf.time = time;
        kf.value = value;
        kf.out_mode = InterpolationMode::Hold;
        kf.in_mode  = InterpolationMode::Hold;
        keyframes.push_back(kf);
    }

    /** Convenience: add a keyframe with an easing preset. */
    void add_eased(double time, const T& value, EasingPreset preset) {
        KeyframeT kf;
        kf.time = time;
        kf.value = value;
        kf.out_mode = InterpolationMode::Linear;
        kf.in_mode  = InterpolationMode::Linear;
        kf.easing   = preset;
        keyframes.push_back(kf);
    }

    /** Ensures keyframes are sorted by ascending time. */
    void sort() {
        std::sort(keyframes.begin(), keyframes.end(),
                  [](const KeyframeT& a, const KeyframeT& b) {
                      return a.time < b.time;
                  });
    }

    [[nodiscard]] bool empty() const { return keyframes.empty(); }
    [[nodiscard]] std::size_t size() const { return keyframes.size(); }

    // --- Evaluation -------------------------------------------------------------

    /**
     * Evaluates the curve at time t.
     *
     * Algorithm:
     *  1. Clamp to boundary keyframes.
     *  2. Binary-search the segment [prev, next].
     *  3. Compute normalised t' = (t - t_prev) / (t_next - t_prev).
     *  4. Apply easing from the OUT keyframe.
     *  5. Interpolate using the out_mode of the prev keyframe.
     */
    [[nodiscard]] T evaluate(double time) const {
        if (keyframes.empty()) return T{};

        // Boundary clamping
        if (keyframes.size() == 1 || time <= keyframes.front().time)
            return keyframes.front().value;
        if (time >= keyframes.back().time)
            return keyframes.back().value;

        // Binary search: find first key with time > requested time
        const auto it = std::upper_bound(
            keyframes.begin(), keyframes.end(), time,
            [](double t, const KeyframeT& kf) { return t < kf.time; });

        assert(it != keyframes.begin());
        assert(it != keyframes.end());

        const KeyframeT& k0 = *(it - 1);
        const KeyframeT& k1 = *it;

        // Hold key: no interpolation
        if (k0.out_mode == InterpolationMode::Hold)
            return k0.value;

        // Normalise t into [0,1]
        const double seg_len = k1.time - k0.time;
        double raw_t = (seg_len > 0.0) ? (time - k0.time) / seg_len : 0.0;

        // Apply easing using k0's preset
        const double eased_t = apply_easing(raw_t, k0.easing, k0.bezier);

        // Choose interpolation
        switch (k0.out_mode) {
            case InterpolationMode::Linear:
                return LerpTraits<T>::lerp(k0.value, k1.value, eased_t);

            case InterpolationMode::Bezier:
                return hermite_interp(k0, k1, eased_t);

            case InterpolationMode::Hold:
                // Already handled above, but silence warning
                return k0.value;
        }

        // Fallback (should be unreachable)
        return LerpTraits<T>::lerp(k0.value, k1.value, eased_t);
    }
};

} // namespace animation
} // namespace tachyon
