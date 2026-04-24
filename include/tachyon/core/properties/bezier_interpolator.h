#pragma once

#include "tachyon/core/animation/easing.h"
#include <vector>
#include <optional>
#include <cstddef>
#include <string>

namespace tachyon::properties {

/**
 * @brief Interpolation type for a keyframe segment.
 * 
 * Explicit, no hidden defaults. Hold means the value stays constant
 * until the next keyframe. Linear means constant velocity.
 * Bezier means cubic-bezier easing with explicit tangents.
 */
enum class InterpolationType {
    Hold,
    Linear,
    Bezier
};

/**
 * @brief A single keyframe for scalar curves.
 * 
 * The tangent and easing data is deterministic and participates in cache keys.
 * This is the canonical keyframe type for all scalar properties.
 */
struct ScalarKeyframe {
    float time{0.0f};
    float value{0.0f};
    float in_tangent{0.0f};   ///< Slope incoming (value units per second).
    float out_tangent{0.0f};  ///< Slope outgoing (value units per second).
    float in_influence{0.333f};  ///< Handle length percentage (0-1).
    float out_influence{0.333f}; ///< Handle length percentage (0-1).
    InterpolationType type{InterpolationType::Linear};
    animation::CubicBezierEasing bezier{animation::CubicBezierEasing::linear()};
};

/**
 * @brief Universal cubic-bezier interpolator for scalar curves.
 * 
 * This is the single interpolation core for all animatable properties:
 * - layer transforms
 * - camera parameters (focal length, aperture, focus distance)
 * - mask controls
 * - text selector amounts
 * - time remap curves
 * 
 * It lives under the property model, not as a timeline-only helper.
 * 
 * Rules:
 * - All evaluation is deterministic: same keyframes + same time = same value.
 * - Cache keys must include the curve shape when visible output depends on it.
 * - The graph editor is a view on this core, not a separate system.
 */
class BezierInterpolator {
public:
    /**
     * @brief Evaluate a scalar curve at time t.
     * 
     * Before the first keyframe: returns first keyframe value.
     * After the last keyframe: returns last keyframe value.
     * Between keyframes: interpolates according to the segment type.
     * 
     * @param keyframes Sorted by time (ascending). Must be validated.
     * @param t Time in seconds.
     * @return The interpolated value.
     */
    [[nodiscard]] static float evaluate(
        const std::vector<ScalarKeyframe>& keyframes,
        float t);
    
    /**
     * @brief Convert a value graph to a speed graph.
     * 
     * Speed = derivative of value with respect to time.
     * This is used by the graph editor backend to show velocity curves.
     */
    [[nodiscard]] static std::vector<ScalarKeyframe> value_to_speed_graph(
        const std::vector<ScalarKeyframe>& value_graph);
    
    /**
     * @brief Convert a speed graph back to a value graph.
     * 
     * Integrates speed over time to reconstruct value.
     * The first keyframe value is preserved as the integration constant.
     */
    [[nodiscard]] static std::vector<ScalarKeyframe> speed_to_value_graph(
        const std::vector<ScalarKeyframe>& speed_graph);
    
    /**
     * @brief Validate a keyframe curve.
     * 
     * Checks:
     * - Keyframes are sorted by time.
     * - No duplicate times.
     * - Bezier segments have valid control points (cx1,cy1,cx2,cy2 in [0,1]).
     * 
     * Returns true if valid. If invalid, fills out_error.
     */
    [[nodiscard]] static bool validate(
        const std::vector<ScalarKeyframe>& keyframes,
        std::string& out_error);

private:
    [[nodiscard]] static float interpolate_linear(
        const ScalarKeyframe& a,
        const ScalarKeyframe& b,
        float t);
    
    [[nodiscard]] static float interpolate_bezier(
        const ScalarKeyframe& a,
        const ScalarKeyframe& b,
        float t);
    
    [[nodiscard]] static std::size_t find_segment(
        const std::vector<ScalarKeyframe>& keyframes,
        float t);
};

} // namespace tachyon::properties
