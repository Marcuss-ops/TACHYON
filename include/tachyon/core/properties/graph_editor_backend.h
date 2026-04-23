#pragma once

#include "tachyon/core/properties/bezier_interpolator.h"
#include "tachyon/core/math/vector2.h"
#include <string>
#include <vector>

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
};

} // namespace tachyon::properties
