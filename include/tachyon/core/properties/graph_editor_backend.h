#pragma once

#include "tachyon/core/properties/bezier_interpolator.h"
#include "tachyon/core/math/vector2.h"
#include <string>
#include <vector>
#include <optional>
#include <functional>

namespace tachyon::properties {

struct GraphPoint {
    float time;
    float value;
    math::Vector2 in_tangent;  ///< Relative to point.
    math::Vector2 out_tangent; ///< Relative to point.
    bool weighted{false};
};

enum class GraphType { Value, Speed, Spatial };

/**
 * @brief Backend logic for the editor's graph view.
 * 
 * Provides a unified way to access and modify animation curves
 * regardless of whether they drive layers, text, or effects.
 */
class GraphEditorBackend {
public:
    /**
     * @brief Get graph points for a scalar track.
     */
    static std::vector<GraphPoint> get_value_graph(const std::vector<ScalarKeyframe>& track);
    
    /**
     * @brief Get velocity points for a scalar track.
     */
    static std::vector<GraphPoint> get_speed_graph(const std::vector<ScalarKeyframe>& track);
    
    /**
     * @brief Apply an easing preset to a keyframe.
     */
    static void apply_preset_ease_in(ScalarKeyframe& kf);
    static void apply_preset_ease_out(ScalarKeyframe& kf);
    static void apply_preset_ease_in_out(ScalarKeyframe& kf);
    
    /**
     * @brief Add a new keyframe to a track at specified time.
     */
    static void add_keyframe(std::vector<ScalarKeyframe>& track, float time, float value);
    
    /**
     * @brief Remove keyframes at specified indices.
     */
    static void remove_keyframes(std::vector<ScalarKeyframe>& track, const std::vector<size_t>& indices);
    
    /**
     * @brief Set tangents for a keyframe (Bezier mode).
     */
    static void set_keyframe_tangents(ScalarKeyframe& kf, 
                                      float in_slope, float out_slope,
                                      float in_influence = 0.33f, float out_influence = 0.33f);
    
    /**
     * @brief Convert keyframe to auto-bezier (smooth).
     */
    static void convert_to_auto_bezier(ScalarKeyframe& kf, 
                                       const std::vector<ScalarKeyframe>& track, size_t index);
    
    /**
     * @brief Convert keyframe to linear interpolation.
     */
    static void convert_to_linear(ScalarKeyframe& kf);
    
    /**
     * @brief Convert keyframe to hold interpolation.
     */
    static void convert_to_hold(ScalarKeyframe& kf);
    
    /**
     * @brief Get the interpolated value at a specific time.
     */
    static float evaluate(const std::vector<ScalarKeyframe>& track, float time);
    
    /**
     * @brief Snap keyframe times to a grid (e.g., frame boundaries).
     */
    static void snap_to_grid(std::vector<ScalarKeyframe>& track, float grid_size);
    
    /**
     * @brief Scale keyframe times around a pivot point.
     */
    static void scale_times(std::vector<ScalarKeyframe>& track, float scale, float pivot_time);
    
    /**
     * @brief Move keyframe times by an offset.
     */
    static void offset_times(std::vector<ScalarKeyframe>& track, float time_offset);
    
    /**
     * @brief Move keyframe values by an offset.
     */
    static void offset_values(std::vector<ScalarKeyframe>& track, float value_offset);
    
    /**
     * @brief Flatten selected keyframes to a constant value.
     */
    static void flatten_values(std::vector<ScalarKeyframe>& track, 
                               const std::vector<size_t>& indices, float target_value);
    
    /**
     * @brief Randomize keyframe values within a range.
     */
    static void randomize_values(std::vector<ScalarKeyframe>& track,
                                 const std::vector<size_t>& indices, 
                                 float min_delta, float max_delta, uint32_t seed);
    
    /**
     * @brief Smooth keyframe values using moving average.
     */
    static void smooth_values(std::vector<ScalarKeyframe>& track,
                              const std::vector<size_t>& indices, int window_size);
    
    /**
     * @brief Simplify curve by removing redundant keyframes (Douglas-Peucker).
     */
    static std::vector<ScalarKeyframe> simplify_curve(const std::vector<ScalarKeyframe>& track, 
                                                       float tolerance);
};

} // namespace tachyon::properties
