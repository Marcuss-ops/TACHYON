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

    // Additional CSS-style presets
    CircIn,     ///< Circular easing in.
    CircOut,    ///< Circular easing out.
    CircInOut,  ///< Circular easing in-out.
    BackIn,     ///< Back easing in (overshoot).
    BackOut,    ///< Back easing out (overshoot).
    BackInOut,  ///< Back easing in-out.

    // Bounce presets
    BounceIn,   ///< Bounce easing in.
    BounceOut,  ///< Bounce easing out (like a ball hitting the ground).
    BounceInOut,///< Bounce easing in-out.

    // Elastic presets
    ElasticIn,  ///< Elastic easing in (spring-like oscillation).
    ElasticOut, ///< Elastic easing out.
    ElasticInOut,///< Elastic easing in-out.

    Custom,     ///< Fully user-defined Bezier control points.
    Spring,     ///< Physics-based spring damping (requires parameters).
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

    // Back easing Bezier approximations (overshoot)
    static CubicBezierEasing back_in()      { return {0.36, 0.0, 0.66, -0.56}; }
    static CubicBezierEasing back_out()     { return {0.34, 1.56, 0.64, 1.0}; }
    static CubicBezierEasing back_in_out()  { return {0.68, -0.6, 0.32, 1.6}; }

    // Circ easing Bezier approximations
    static CubicBezierEasing circ_in()     { return {0.55, 0.0, 1.0, 0.45}; }
    static CubicBezierEasing circ_out()     { return {0.0, 0.55, 0.45, 1.0}; }
    static CubicBezierEasing circ_in_out()  { return {0.785, 0.0, 0.215, 1.0}; }

    // Bounce easing (requires special functions, not bezier)
    // Elastic easing Bezier approximations
    static CubicBezierEasing elastic_in()   { return {0.66, 0.0, 0.34, 1.0}; }
    static CubicBezierEasing elastic_out()  { return {0.0, 0.0, 0.58, 1.0}; }
    static CubicBezierEasing elastic_in_out(){ return {0.66, 0.0, 0.34, 1.0}; }

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
 * Spring easing parameters for physics-based animation.
 * Uses a damped harmonic oscillator model.
 */
struct SpringEasing {
    double stiffness{180.0};  ///< Spring stiffness (k)
    double damping{12.0};     ///< Damping coefficient
    double mass{1.0};         ///< Mass of the object
    double velocity{0.0};     ///< Initial velocity

    /**
     * Evaluate spring damping at time t in [0,1].
     * Returns a value in [0,1] representing the damped oscillation.
     */
    [[nodiscard]] double evaluate(double t) const {
        if (t <= 0.0) return 0.0;
        if (t >= 1.0) return 1.0;

        const double duration = 1.0;

        // Damped harmonic oscillator solution
        const double m = mass;
        const double k = stiffness;
        const double c = damping;

        const double c_critical = 2.0 * std::sqrt(k * m);
        const double c_normalized = c / c_critical;

        if (c_normalized < 1.0) {
            // Underdamped: oscillatory
            const double omega = std::sqrt(k / m - (c * c) / (4.0 * m * m));
            const double decay = std::exp(-c * t / (2.0 * m));
            return 1.0 - decay * std::cos(omega * t * duration);
        } else if (c_normalized > 1.0) {
            // Overdamped: no oscillation
            const double r1 = (-c + std::sqrt(c * c - 4.0 * k * m)) / (2.0 * m);
            const double r2 = (-c - std::sqrt(c * c - 4.0 * k * m)) / (2.0 * m);
            const double coeff = (r2 * std::exp(r1 * t * duration) - r1 * std::exp(r2 * t * duration)) / (r2 - r1);
            return 1.0 - std::exp(-c_normalized * t) * coeff;
        } else {
            // Critically damped
            const double r = -c / (2.0 * m);
            return 1.0 - std::exp(r * t * duration) * (1.0 + r * t * duration);
        }
    }
};

// --- Bounce easing math functions ---

[[nodiscard]] inline double bounce_out(double t) {
    if (t < 1.0 / 2.75) {
        return 7.5625 * t * t;
    } else if (t < 2.0 / 2.75) {
        t -= 1.5 / 2.75;
        return 7.5625 * t * t + 0.75;
    } else if (t < 2.5 / 2.75) {
        t -= 2.25 / 2.75;
        return 7.5625 * t * t + 0.9375;
    } else {
        t -= 2.625 / 2.75;
        return 7.5625 * t * t + 0.984375;
    }
}

[[nodiscard]] inline double bounce_in(double t) {
    return 1.0 - bounce_out(1.0 - t);
}

[[nodiscard]] inline double bounce_in_out(double t) {
    if (t < 0.5) {
        return 0.5 * (1.0 - bounce_out(1.0 - 2.0 * t));
    } else {
        return 0.5 * (1.0 + bounce_out(2.0 * t - 1.0));
    }
}

// --- Elastic easing math functions ---

[[nodiscard]] inline double elastic_out(double t) {
    if (t == 0.0 || t == 1.0) return t;
    const double pi = 3.14159265358979323846;
    return std::pow(2.0, -10.0 * t) * std::sin((t * 10.0 - 0.75) * (2.0 * pi / 3.0)) + 1.0;
}

[[nodiscard]] inline double elastic_in(double t) {
    return 1.0 - elastic_out(1.0 - t);
}

[[nodiscard]] inline double elastic_in_out(double t) {
    if (t < 0.5) {
        return 0.5 * elastic_in(t * 2.0);
    } else {
        return 0.5 * (1.0 + elastic_out((t - 0.5) * 2.0));
    }
}

// --- Back easing math functions ---

[[nodiscard]] inline double back_in(double t) {
    const double s = 1.70158;
    return t * t * ((s + 1.0) * t - s);
}

[[nodiscard]] inline double back_out(double t) {
    const double s = 1.70158;
    t -= 1.0;
    return t * t * ((s + 1.0) * t + s) + 1.0;
}

[[nodiscard]] inline double back_in_out(double t) {
    const double s = 1.70158 * 1.525;
    if (t < 0.5) {
        t *= 2.0;
        return 0.5 * (t * t * ((s + 1.0) * t - s));
    } else {
        t = 2.0 * t - 2.0;
        return 0.5 * (t * t * ((s + 1.0) * t + s) + 2.0);
    }
}

// --- Circ easing math functions ---

[[nodiscard]] inline double circ_in(double t) {
    return 1.0 - std::sqrt(1.0 - t * t);
}

[[nodiscard]] inline double circ_out(double t) {
    t -= 1.0;
    return std::sqrt(1.0 - t * t);
}

[[nodiscard]] inline double circ_in_out(double t) {
    if (t < 0.5) {
        return 0.5 * (1.0 - std::sqrt(1.0 - 4.0 * t * t));
    } else {
        t = 2.0 * t - 2.0;
        return 0.5 * (std::sqrt(1.0 - t * t) + 1.0);
    }
}

/**
 * Applies the correct easing for the given preset to a raw t in [0,1].
 * Returns a remapped t' in [0,1].
 */
[[nodiscard]] inline double apply_easing(double t, EasingPreset preset, const CubicBezierEasing& custom = {},
                                        const SpringEasing& spring = SpringEasing{}) {
    switch (preset) {
        case EasingPreset::None:      return t;
        case EasingPreset::EaseIn:    return CubicBezierEasing::ease_in().evaluate(t);
        case EasingPreset::EaseOut:   return CubicBezierEasing::ease_out().evaluate(t);
        case EasingPreset::EaseInOut: return CubicBezierEasing::ease_in_out().evaluate(t);
        case EasingPreset::Custom:    return custom.evaluate(t);

        // Circ easing
        case EasingPreset::CircIn:    return circ_in(t);
        case EasingPreset::CircOut:   return circ_out(t);
        case EasingPreset::CircInOut: return circ_in_out(t);

        // Back easing
        case EasingPreset::BackIn:    return back_in(t);
        case EasingPreset::BackOut:   return back_out(t);
        case EasingPreset::BackInOut: return back_in_out(t);

        // Bounce easing
        case EasingPreset::BounceIn:   return bounce_in(t);
        case EasingPreset::BounceOut:  return bounce_out(t);
        case EasingPreset::BounceInOut:return bounce_in_out(t);

        // Elastic easing
        case EasingPreset::ElasticIn:   return elastic_in(t);
        case EasingPreset::ElasticOut:  return elastic_out(t);
        case EasingPreset::ElasticInOut:return elastic_in_out(t);

        // Spring easing
        case EasingPreset::Spring:     return spring.evaluate(t);
    }
    return t;
}

/**
 * Ease namespace provides factory functions for creating easing specifications.
 * Usage: Ease::Bounce(), Ease::Spring(180, 12), Ease::Cubic(0.34, 1.56, 0.64, 1.0)
 */
namespace Ease {

    /** Create a CubicBezierEasing with custom control points. */
    [[nodiscard]] inline CubicBezierEasing Cubic(double cx1, double cy1, double cx2, double cy2) {
        return {cx1, cy1, cx2, cy2};
    }

    /** Create a SpringEasing with stiffness and damping. */
    [[nodiscard]] inline SpringEasing Spring(double stiffness = 180.0, double damping = 12.0, double mass = 1.0) {
        return {stiffness, damping, mass, 0.0};
    }

    // Preset factory functions
    [[nodiscard]] inline EasingPreset Bounce()      { return EasingPreset::BounceOut; }
    [[nodiscard]] inline EasingPreset BounceIn()    { return EasingPreset::BounceIn; }
    [[nodiscard]] inline EasingPreset BounceOut()   { return EasingPreset::BounceOut; }
    [[nodiscard]] inline EasingPreset BounceInOut() { return EasingPreset::BounceInOut; }

    [[nodiscard]] inline EasingPreset Elastic()      { return EasingPreset::ElasticOut; }
    [[nodiscard]] inline EasingPreset ElasticIn()    { return EasingPreset::ElasticIn; }
    [[nodiscard]] inline EasingPreset ElasticOut()   { return EasingPreset::ElasticOut; }
    [[nodiscard]] inline EasingPreset ElasticInOut() { return EasingPreset::ElasticInOut; }

    [[nodiscard]] inline EasingPreset Back()      { return EasingPreset::BackOut; }
    [[nodiscard]] inline EasingPreset BackIn()    { return EasingPreset::BackIn; }
    [[nodiscard]] inline EasingPreset BackOut()   { return EasingPreset::BackOut; }
    [[nodiscard]] inline EasingPreset BackInOut() { return EasingPreset::BackInOut; }

    [[nodiscard]] inline EasingPreset Circ()      { return EasingPreset::CircOut; }
    [[nodiscard]] inline EasingPreset CircIn()    { return EasingPreset::CircIn; }
    [[nodiscard]] inline EasingPreset CircOut()   { return EasingPreset::CircOut; }
    [[nodiscard]] inline EasingPreset CircInOut() { return EasingPreset::CircInOut; }

} // namespace Ease

} // namespace animation
} // namespace tachyon
