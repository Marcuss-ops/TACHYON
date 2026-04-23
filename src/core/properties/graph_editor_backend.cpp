#include "tachyon/core/properties/graph_editor_backend.h"
#include <algorithm>

namespace tachyon::properties {

// Bezier control point constants (replacing magic numbers)
namespace {
    constexpr float BEZIER_INFLUENCE = 0.1f;          // Horizontal influence for tangents
    constexpr float EASE_IN_CX1 = 0.33f;            // Ease In: first control point x
    constexpr float EASE_IN_CY1 = 0.0f;             // Ease In: first control point y
    constexpr float EASE_IN_CX2 = 0.66f;            // Ease In: second control point x
    constexpr float EASE_IN_CY2 = 1.0f;             // Ease In: second control point y
    constexpr float EASE_OUT_CX1 = 0.0f;            // Ease Out: first control point x
    constexpr float EASE_OUT_CY1 = 0.0f;            // Ease Out: first control point y
    constexpr float EASE_OUT_CX2 = 0.33f;           // Ease Out: second control point x
    constexpr float EASE_OUT_CY2 = 1.0f;            // Ease Out: second control point y
    constexpr float EASE_IN_OUT_CX1 = 0.33f;        // Ease In-Out: first control point x
    constexpr float EASE_IN_OUT_CY1 = 0.0f;         // Ease In-Out: first control point y
    constexpr float EASE_IN_OUT_CX2 = 0.33f;        // Ease In-Out: second control point x
    constexpr float EASE_IN_OUT_CY2 = 1.0f;         // Ease In-Out: second control point y
} // namespace

std::vector<GraphPoint> GraphEditorBackend::get_value_graph(const std::vector<ScalarKeyframe>& track) {
    std::vector<GraphPoint> points;
    points.reserve(track.size());
    
    for (const auto& kf : track) {
        GraphPoint p;
        p.time = kf.time;
        p.value = kf.value;
        
        // Convert slope tangents to relative vectors for the UI
        // AE-style: horizontal length is influence, vertical is speed
        p.in_tangent = {-BEZIER_INFLUENCE, -kf.in_tangent * BEZIER_INFLUENCE};
        p.out_tangent = {BEZIER_INFLUENCE, kf.out_tangent * BEZIER_INFLUENCE};
        
        points.push_back(p);
    }
    return points;
}

std::vector<GraphPoint> GraphEditorBackend::get_speed_graph(const std::vector<ScalarKeyframe>& track) {
    auto speeds = BezierInterpolator::value_to_speed_graph(track);
    return get_value_graph(speeds);
}

void GraphEditorBackend::apply_preset_ease_in(ScalarKeyframe& kf) {
    kf.type = InterpolationType::Bezier;
    kf.bezier.cx1 = EASE_IN_CX1; kf.bezier.cy1 = EASE_IN_CY1;
    kf.bezier.cx2 = EASE_IN_CX2; kf.bezier.cy2 = EASE_IN_CY2;
}

void GraphEditorBackend::apply_preset_ease_out(ScalarKeyframe& kf) {
    kf.type = InterpolationType::Bezier;
    kf.bezier.cx1 = EASE_OUT_CX1;  kf.bezier.cy1 = EASE_OUT_CY1;
    kf.bezier.cx2 = EASE_OUT_CX2; kf.bezier.cy2 = EASE_OUT_CY2;
}

void GraphEditorBackend::apply_preset_ease_in_out(ScalarKeyframe& kf) {
    kf.type = InterpolationType::Bezier;
    kf.bezier.cx1 = EASE_IN_OUT_CX1; kf.bezier.cy1 = EASE_IN_OUT_CY1;
    kf.bezier.cx2 = EASE_IN_OUT_CX2; kf.bezier.cy2 = EASE_IN_OUT_CY2;
}

} // namespace tachyon::properties
