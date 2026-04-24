#include "tachyon/core/properties/graph_editor_backend.h"
#include <algorithm>
#include <cmath>
#include <random>
#include <numeric>

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

void GraphEditorBackend::add_keyframe(std::vector<ScalarKeyframe>& track, float time, float value) {
    ScalarKeyframe kf;
    kf.time = time;
    kf.value = value;
    kf.type = InterpolationType::Linear;
    
    // Insert in sorted order
    auto it = std::lower_bound(track.begin(), track.end(), time,
        [](const ScalarKeyframe& a, float t) { return a.time < t; });
    track.insert(it, kf);
}

void GraphEditorBackend::remove_keyframes(std::vector<ScalarKeyframe>& track, const std::vector<size_t>& indices) {
    if (indices.empty()) return;
    
    // Sort indices in descending order to remove from end first
    std::vector<size_t> sorted_indices = indices;
    std::sort(sorted_indices.rbegin(), sorted_indices.rend());
    
    for (size_t idx : sorted_indices) {
        if (idx < track.size()) {
            track.erase(track.begin() + idx);
        }
    }
}

void GraphEditorBackend::set_keyframe_tangents(ScalarKeyframe& kf, 
                                                float in_slope, float out_slope,
                                                float in_influence, float out_influence) {
    kf.type = InterpolationType::Bezier;
    kf.in_tangent = in_slope;
    kf.out_tangent = out_slope;
    kf.bezier.in_influence = in_influence;
    kf.bezier.out_influence = out_influence;
}

void GraphEditorBackend::convert_to_auto_bezier(ScalarKeyframe& kf, 
                                                 const std::vector<ScalarKeyframe>& track, size_t index) {
    if (index >= track.size()) return;
    
    kf.type = InterpolationType::Bezier;
    
    // Calculate smooth tangents based on neighboring keyframes
    float left_time = (index > 0) ? track[index - 1].time : kf.time - 1.0f;
    float right_time = (index < track.size() - 1) ? track[index + 1].time : kf.time + 1.0f;
    float left_value = (index > 0) ? track[index - 1].value : kf.value;
    float right_value = (index < track.size() - 1) ? track[index + 1].value : kf.value;
    
    // Smooth tangent: average of slopes to neighbors
    float left_slope = (kf.time - left_time > 1e-6f) ? (kf.value - left_value) / (kf.time - left_time) : 0.0f;
    float right_slope = (right_time - kf.time > 1e-6f) ? (right_value - kf.value) / (right_time - kf.time) : 0.0f;
    
    float avg_slope = (left_slope + right_slope) * 0.5f;
    kf.in_tangent = avg_slope;
    kf.out_tangent = avg_slope;
    
    // Auto-bezier uses symmetric control points
    kf.bezier.cx1 = 0.33f; kf.bezier.cy1 = 0.0f;
    kf.bezier.cx2 = 0.66f; kf.bezier.cy2 = 1.0f;
    kf.bezier.weighted = false;
}

void GraphEditorBackend::convert_to_linear(ScalarKeyframe& kf) {
    kf.type = InterpolationType::Linear;
}

void GraphEditorBackend::convert_to_hold(ScalarKeyframe& kf) {
    kf.type = InterpolationType::Hold;
}

float GraphEditorBackend::evaluate(const std::vector<ScalarKeyframe>& track, float time) {
    if (track.empty()) return 0.0f;
    if (track.size() == 1) return track[0].value;
    
    // Before first keyframe
    if (time <= track.front().time) return track.front().value;
    
    // After last keyframe
    if (time >= track.back().time) return track.back().value;
    
    // Find segment
    auto it = std::lower_bound(track.begin(), track.end(), time,
        [](const ScalarKeyframe& a, float t) { return a.time < t; });
    
    if (it == track.begin() || it == track.end()) return 0.0f;
    
    const auto& prev = *(it - 1);
    const auto& curr = *it;
    
    return BezierInterpolator::interpolate_scalar(prev, curr, time);
}

void GraphEditorBackend::snap_to_grid(std::vector<ScalarKeyframe>& track, float grid_size) {
    if (grid_size <= 0.0f) return;
    
    for (auto& kf : track) {
        kf.time = std::round(kf.time / grid_size) * grid_size;
    }
    
    // Re-sort after snapping
    std::sort(track.begin(), track.end(),
        [](const ScalarKeyframe& a, const ScalarKeyframe& b) { return a.time < b.time; });
}

void GraphEditorBackend::scale_times(std::vector<ScalarKeyframe>& track, float scale, float pivot_time) {
    if (scale <= 0.0f) return;
    
    for (auto& kf : track) {
        kf.time = pivot_time + (kf.time - pivot_time) * scale;
    }
}

void GraphEditorBackend::offset_times(std::vector<ScalarKeyframe>& track, float time_offset) {
    if (time_offset == 0.0f) return;
    
    for (auto& kf : track) {
        kf.time += time_offset;
    }
    
    // Re-sort after offset
    std::sort(track.begin(), track.end(),
        [](const ScalarKeyframe& a, const ScalarKeyframe& b) { return a.time < b.time; });
}

void GraphEditorBackend::offset_values(std::vector<ScalarKeyframe>& track, float value_offset) {
    if (value_offset == 0.0f) return;
    
    for (auto& kf : track) {
        kf.value += value_offset;
    }
}

void GraphEditorBackend::flatten_values(std::vector<ScalarKeyframe>& track, 
                                         const std::vector<size_t>& indices, float target_value) {
    for (size_t idx : indices) {
        if (idx < track.size()) {
            track[idx].value = target_value;
        }
    }
}

void GraphEditorBackend::randomize_values(std::vector<ScalarKeyframe>& track,
                                           const std::vector<size_t>& indices, 
                                           float min_delta, float max_delta, uint32_t seed) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(min_delta, max_delta);
    
    for (size_t idx : indices) {
        if (idx < track.size()) {
            track[idx].value += dist(rng);
        }
    }
}

void GraphEditorBackend::smooth_values(std::vector<ScalarKeyframe>& track,
                                        const std::vector<size_t>& indices, int window_size) {
    if (window_size < 1 || indices.empty()) return;
    
    // Create a copy of original values
    std::vector<float> original_values;
    original_values.reserve(track.size());
    for (const auto& kf : track) {
        original_values.push_back(kf.value);
    }
    
    // Apply moving average to selected indices
    for (size_t idx : indices) {
        if (idx >= track.size()) continue;
        
        float sum = 0.0f;
        int count = 0;
        
        for (int i = static_cast<int>(idx) - window_size / 2; 
             i <= static_cast<int>(idx) + window_size / 2; ++i) {
            if (i >= 0 && static_cast<size_t>(i) < track.size()) {
                sum += original_values[i];
                ++count;
            }
        }
        
        if (count > 0) {
            track[idx].value = sum / static_cast<float>(count);
        }
    }
}

std::vector<ScalarKeyframe> GraphEditorBackend::simplify_curve(const std::vector<ScalarKeyframe>& track, 
                                                                float tolerance) {
    if (track.size() <= 2) return track;
    
    // Douglas-Peucker algorithm for curve simplification
    std::vector<bool> keep(track.size(), false);
    keep[0] = true;
    keep[track.size() - 1] = true;
    
    std::function<void(size_t, size_t)> simplify_recursive = [&](size_t start, size_t end) {
        if (end <= start + 1) return;
        
        // Find perpendicular distance from line (start, end) to all intermediate points
        float max_dist = 0.0f;
        size_t max_idx = start;
        
        const auto& p1 = track[start];
        const auto& p2 = track[end];
        
        const float dx = p2.time - p1.time;
        const float dy = p2.value - p1.value;
        const float len_sq = dx * dx + dy * dy;
        
        for (size_t i = start + 1; i < end; ++i) {
            const float t = std::clamp(((track[i].time - p1.time) * dx + 
                                        (track[i].value - p1.value) * dy) / len_sq, 0.0f, 1.0f);
            const float proj_x = p1.time + t * dx;
            const float proj_y = p1.value + t * dy;
            const float dist = std::hypot(track[i].time - proj_x, track[i].value - proj_y);
            
            if (dist > max_dist) {
                max_dist = dist;
                max_idx = i;
            }
        }
        
        if (max_dist > tolerance) {
            keep[max_idx] = true;
            simplify_recursive(start, max_idx);
            simplify_recursive(max_idx, end);
        }
    };
    
    simplify_recursive(0, track.size() - 1);
    
    // Build result with kept keyframes
    std::vector<ScalarKeyframe> result;
    for (size_t i = 0; i < track.size(); ++i) {
        if (keep[i]) {
            result.push_back(track[i]);
        }
    }
    
    return result;
}

} // namespace tachyon::properties
