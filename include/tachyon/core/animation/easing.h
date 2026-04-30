#pragma once

#include <cmath>
#include <algorithm>

namespace tachyon {
namespace animation {

/**
 * Supported easing presets applied on top of interpolation.
 */
enum class EasingPreset {
    None,       ///< No easing — raw interpolant is used as-is.
    EaseIn,     ///< Slow start, fast finish (cubic in).
    EaseOut,    ///< Fast start, slow finish (cubic out).
    EaseInOut,  ///< Slow start and finish (cubic symmetric).
    Custom,     ///< Fully user-defined Bezier control points.
};

/**
 * Cubic Bezier easing — solves for t given a normalized x in [0,1]
 * using the unit-Bezier with two control points (cx1,cy1) and (cx2,cy2).
 * This matches After Effects and CSS cubic-bezier convention.
 */
struct CubicBezierEasing {
    double cx1{0.0}; ///< x of control point 1
    double cy1{0.0}; ///< y of control point 1
    double cx2{1.0}; ///< x of control point 2
    double cy2{1.0}; ///< y of control point 2

    /**
     * Evaluate the Bezier at a given progress x in [0,1].
     * Solves for parameter t via Newton-Raphson, then returns y(t).
     */
    [[nodiscard]] double evaluate(double x) const {
        // Bezier polynomial coefficients
        const double ax = 3.0 * cx1 - 3.0 * cx2 + 1.0;
        const double bx = 3.0 * cx2 - 6.0 * cx1;
        const double cx = 3.0 * cx1;

        const double ay = 3.0 * cy1 - 3.0 * cy2 + 1.0;
        const double by = 3.0 * cy2 - 6.0 * cy1;
        const double cy = 3.0 * cy1;

        // Newton-Raphson to solve x(t) = x
        double t = x; // Initial guess
        for (int i = 0; i < 8; ++i) {
            const double x_t = ((ax * t + bx) * t + cx) * t;
            const double dx_t = (3.0 * ax * t + 2.0 * bx) * t + cx;
            if (std::abs(dx_t) < 1e-10) break;
            t -= (x_t - x) / dx_t;
            if (t < 0.0) t = 0.0;
            if (t > 1.0) t = 1.0;
        }

        return ((ay * t + by) * t + cy) * t;
    }

    /** Standard presets matching CSS / After Effects conventions. */
    static CubicBezierEasing ease_in()     { return {0.42, 0.0, 1.0,  1.0}; }
    static CubicBezierEasing ease_out()    { return {0.0,  0.0, 0.58, 1.0}; }
    static CubicBezierEasing ease_in_out() { return {0.42, 0.0, 0.58, 1.0}; }
    static CubicBezierEasing linear()      { return {0.0,  0.0, 1.0,  1.0}; }

    /**
     * Create from After Effects speed and influence parameters.
     * @param speed_out     Pixels/Units per second leaving the first keyframe.
     * @param influence_out Percentage (0-100) of the segment time the handle influences.
     * @param speed_in      Pixels/Units per second entering the second keyframe.
     * @param influence_in  Percentage (0-100) of the segment time the handle influences.
     * @param duration      Time in seconds between keyframes.
     * @param value_delta   Total change in value between keyframes.
     */
    static CubicBezierEasing from_ae(double speed_out, double influence_out,
                                     double speed_in, double influence_in,
                                     double duration, double value_delta) {
        const double x1 = std::clamp(influence_out / 100.0, 0.0, 1.0);
        const double x2 = 1.0 - std::clamp(influence_in / 100.0, 0.0, 1.0);
        
        double y1 = 0.0;
        double y2 = 1.0;
        
        if (std::abs(value_delta) > 1e-8) {
            y1 = std::clamp((speed_out * duration * x1) / value_delta, 0.0, 1.0);
            y2 = std::clamp(1.0 - (speed_in * duration * (1.0 - x2)) / value_delta, 0.0, 1.0);
        }
        
        return {x1, y1, x2, y2};
    }
};

/**
 * Applies the correct easing for the given preset to a raw t in [0,1].
 * Returns a remapped t' in [0,1].
 */
[[nodiscard]] inline double apply_easing(double t, EasingPreset preset, const CubicBezierEasing& custom = {}) {
    switch (preset) {
        case EasingPreset::None:      return t;
        case EasingPreset::EaseIn:    return CubicBezierEasing::ease_in().evaluate(t);
        case EasingPreset::EaseOut:   return CubicBezierEasing::ease_out().evaluate(t);
        case EasingPreset::EaseInOut: return CubicBezierEasing::ease_in_out().evaluate(t);
        case EasingPreset::Custom:    return custom.evaluate(t);
    }
    return t;
}

} // namespace animation
} // namespace tachyon
