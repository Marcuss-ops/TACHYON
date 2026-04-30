#include "tachyon/core/properties/bezier_interpolator.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace tachyon::properties {

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::size_t BezierInterpolator::find_segment(
    const std::vector<ScalarKeyframe>& keyframes,
    float t) {
    
    if (keyframes.empty()) return 0;
    if (t <= keyframes.front().time) return 0;
    if (t >= keyframes.back().time) return keyframes.size() - 1;
    
    auto it = std::upper_bound(
        keyframes.begin(), keyframes.end(), t,
        [](float time, const ScalarKeyframe& kf) { return time < kf.time; });
    
    if (it == keyframes.begin()) return 0;
    return static_cast<std::size_t>(it - keyframes.begin() - 1);
}

float BezierInterpolator::interpolate_linear(
    const ScalarKeyframe& a,
    const ScalarKeyframe& b,
    float t) {
    
    const float duration = b.time - a.time;
    if (duration <= 1e-8f) return a.value;
    
    const float u = (t - a.time) / duration;
    return a.value + u * (b.value - a.value);
}

float BezierInterpolator::interpolate_bezier(
    const ScalarKeyframe& a,
    const ScalarKeyframe& b,
    float t) {
    
    const float duration = b.time - a.time;
    if (duration <= 1e-8f) return a.value;
    
    const float dv = b.value - a.value;
    const float x = (t - a.time) / duration;
    
    // Weighted Tangents Mapping (AE Influence/Speed to Cubic Bezier)
    animation::CubicBezierEasing segment_bezier = a.bezier;
    
    // cx1/cx2 are normalized influences
    segment_bezier.cx1 = std::clamp(a.out_influence, 0.0f, 1.0f);
    segment_bezier.cx2 = 1.0f - std::clamp(b.in_influence, 0.0f, 1.0f);
    
    if (std::abs(dv) > 1e-6f) {
        // cy1/cy2 are derived from slopes (tangents)
        segment_bezier.cy1 = (a.out_tangent * (duration * segment_bezier.cx1)) / dv;
        segment_bezier.cy2 = 1.0f - (b.in_tangent * (duration * (1.0f - segment_bezier.cx2))) / dv;
    } else {
        segment_bezier.cy1 = 0.0f;
        segment_bezier.cy2 = 1.0f;
    }

    const float eased_t = static_cast<float>(segment_bezier.evaluate(static_cast<double>(x)));
    return a.value + eased_t * dv;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

float BezierInterpolator::evaluate(
    const std::vector<ScalarKeyframe>& keyframes,
    float t) {
    
    if (keyframes.empty()) return 0.0f;
    if (keyframes.size() == 1) return keyframes.front().value;
    
    // Before first keyframe
    if (t <= keyframes.front().time) {
        return keyframes.front().value;
    }
    
    // After last keyframe
    if (t >= keyframes.back().time) {
        return keyframes.back().value;
    }
    
    const std::size_t seg = find_segment(keyframes, t);
    const ScalarKeyframe& a = keyframes[seg];
    const ScalarKeyframe& b = keyframes[seg + 1];
    
    switch (a.type) {
        case InterpolationType::Hold:
            return a.value;
        case InterpolationType::Linear:
            return interpolate_linear(a, b, t);
        case InterpolationType::Bezier:
            return interpolate_bezier(a, b, t);
    }
    
    return a.value; // unreachable
}

std::vector<ScalarKeyframe> BezierInterpolator::value_to_speed_graph(
    const std::vector<ScalarKeyframe>& value_graph) {
    
    if (value_graph.size() < 2) return {};
    
    std::vector<ScalarKeyframe> speed_graph;
    speed_graph.reserve(value_graph.size());
    
    for (std::size_t i = 0; i < value_graph.size(); ++i) {
        ScalarKeyframe speed_kf;
        speed_kf.time = value_graph[i].time;
        
        if (i == 0) {
            // Forward difference at start
            const float dt = value_graph[1].time - value_graph[0].time;
            speed_kf.value = (dt > 1e-8f) 
                ? (value_graph[1].value - value_graph[0].value) / dt 
                : 0.0f;
        } else if (i == value_graph.size() - 1) {
            // Backward difference at end
            const float dt = value_graph[i].time - value_graph[i - 1].time;
            speed_kf.value = (dt > 1e-8f)
                ? (value_graph[i].value - value_graph[i - 1].value) / dt
                : 0.0f;
        } else {
            // Central difference in middle
            const float dt_prev = value_graph[i].time - value_graph[i - 1].time;
            const float dt_next = value_graph[i + 1].time - value_graph[i].time;
            const float v_prev = (dt_prev > 1e-8f)
                ? (value_graph[i].value - value_graph[i - 1].value) / dt_prev
                : 0.0f;
            const float v_next = (dt_next > 1e-8f)
                ? (value_graph[i + 1].value - value_graph[i].value) / dt_next
                : 0.0f;
            speed_kf.value = (v_prev + v_next) * 0.5f;
        }
        
        speed_kf.type = InterpolationType::Linear;
        speed_graph.push_back(speed_kf);
    }
    
    return speed_graph;
}

std::vector<ScalarKeyframe> BezierInterpolator::speed_to_value_graph(
    const std::vector<ScalarKeyframe>& speed_graph) {
    
    if (speed_graph.empty()) return {};
    if (speed_graph.size() == 1) {
        ScalarKeyframe kf;
        kf.time = speed_graph[0].time;
        kf.value = speed_graph[0].value; // Integration constant = speed value
        return {kf};
    }
    
    std::vector<ScalarKeyframe> value_graph;
    value_graph.reserve(speed_graph.size());
    
    // Integration constant: first keyframe value
    float accumulated_value = speed_graph[0].value;
    
    for (std::size_t i = 0; i < speed_graph.size(); ++i) {
        ScalarKeyframe kf;
        kf.time = speed_graph[i].time;
        
        if (i == 0) {
            kf.value = accumulated_value;
        } else {
            // Trapezoidal integration
            const float dt = speed_graph[i].time - speed_graph[i - 1].time;
            const float avg_speed = (speed_graph[i - 1].value + speed_graph[i].value) * 0.5f;
            accumulated_value += avg_speed * dt;
            kf.value = accumulated_value;
        }
        
        kf.type = InterpolationType::Linear;
        value_graph.push_back(kf);
    }
    
    return value_graph;
}

bool BezierInterpolator::validate(
    const std::vector<ScalarKeyframe>& keyframes,
    std::string& out_error) {
    
    if (keyframes.empty()) return true;
    
    // Check sorted and no duplicate times
    for (std::size_t i = 1; i < keyframes.size(); ++i) {
        if (keyframes[i].time < keyframes[i - 1].time) {
            std::ostringstream oss;
            oss << "Keyframes out of order: keyframe " << i 
                << " has time " << keyframes[i].time
                << " but previous has time " << keyframes[i - 1].time;
            out_error = oss.str();
            return false;
        }
        
        if (std::abs(keyframes[i].time - keyframes[i - 1].time) < 1e-8f) {
            std::ostringstream oss;
            oss << "Duplicate keyframe time at index " << i 
                << ": time = " << keyframes[i].time;
            out_error = oss.str();
            return false;
        }
    }
    
    // Check bezier control points
    for (std::size_t i = 0; i < keyframes.size(); ++i) {
        if (keyframes[i].type == InterpolationType::Bezier) {
            const auto& b = keyframes[i].bezier;
            if (b.cx1 < 0.0 || b.cx1 > 1.0 || b.cx2 < 0.0 || b.cx2 > 1.0 ||
                b.cy1 < 0.0 || b.cy1 > 1.0 || b.cy2 < 0.0 || b.cy2 > 1.0) {
                std::ostringstream oss;
                oss << "Invalid bezier control points at keyframe " << i
                    << ": (" << b.cx1 << ", " << b.cy1 << ", " 
                    << b.cx2 << ", " << b.cy2 << ")";
                out_error = oss.str();
                return false;
            }
        }
    }
    
    return true;
}

} // namespace tachyon::properties
