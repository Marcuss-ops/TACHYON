#pragma once

#include "tachyon/core/animation/easing.h"

namespace tachyon {
namespace animation {

/**
 * Supported interpolation modes between two keyframes.
 */
enum class InterpolationMode {
    Hold,    ///< Value jumps at the OUT keyframe, no blending.
    Linear,  ///< Uniform linear blend between values.
    Bezier,  ///< Cubic Bezier with per-keyframe control points.
};

struct SpringEasing {
    double stiffness{200.0};
    double damping{20.0};
    double mass{1.0};
    double velocity{0.0};
};

/**
 * A single keyframe binding a time position to a typed value.
 *
 * @tparam T  The value type (float, Vector2, Vector3, etc.)
 *
 * Notes:
 *  - `out_mode` determines how to interpolate FROM this keyframe INTO the next.
 *  - `in_mode`  determines which tangent/easing to apply when ARRIVING here.
 *  - Bezier control points (left/right tangent handles) are stored in normalised
 *    time + value deltas relative to this keyframe, matching AE conventions.
 */
template <typename T>
struct Keyframe {
    double           time{0.0};
    T                value{};
    InterpolationMode in_mode{InterpolationMode::Linear};
    InterpolationMode out_mode{InterpolationMode::Linear};
    EasingPreset      easing{EasingPreset::None};
    CubicBezierEasing bezier{CubicBezierEasing::linear()};
    SpringEasing      spring{};

    /**
     * Tangent handle for Bezier curves, expressed as a delta from this keyframe.
     * out_tangent: exit handle (time_delta, value_delta) from this key.
     * in_tangent:  entry handle applied on the NEXT key's side.
     *
     * Only meaningful for Bezier out_mode / in_mode.
     * time component must be >= 0 (handles must not go back in time).
     */
    double out_tangent_time{0.0};
    T      out_tangent_value{};
    double in_tangent_time{0.0};
    T      in_tangent_value{};
};

} // namespace animation
} // namespace tachyon
