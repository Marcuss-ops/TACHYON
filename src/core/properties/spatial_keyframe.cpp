#include "tachyon/core/properties/spatial_keyframe.h"
#include <algorithm>
#include <cmath>

namespace tachyon::properties {

std::size_t SpatialInterpolator::find_segment(
    const std::vector<SpatialKeyframe>& keyframes,
    float t) {
    
    if (keyframes.empty()) return 0;
    if (t <= keyframes.front().time) return 0;
    if (t >= keyframes.back().time) return keyframes.size() - 1;
    
    auto it = std::upper_bound(
        keyframes.begin(), keyframes.end(), t,
        [](float time, const SpatialKeyframe& kf) { return time < kf.time; });
    
    if (it == keyframes.begin()) return 0;
    return static_cast<std::size_t>(it - keyframes.begin() - 1);
}

math::Vector3 SpatialInterpolator::evaluate(
    const std::vector<SpatialKeyframe>& keyframes,
    float t) {
    
    if (keyframes.empty()) return {0,0,0};
    if (keyframes.size() == 1) return keyframes.front().position;
    
    if (t <= keyframes.front().time) return keyframes.front().position;
    if (t >= keyframes.back().time) return keyframes.back().position;
    
    const std::size_t seg_idx = find_segment(keyframes, t);
    const auto& a = keyframes[seg_idx];
    const auto& b = keyframes[seg_idx + 1];
    
    const float duration = b.time - a.time;
    if (duration <= 1e-8f) return a.position;
    
    const float u = (t - a.time) / duration;
    
    if (a.linear) {
        return a.position * (1.0f - u) + b.position * u;
    }
    
    // Cubic Hermite Spline evaluation
    // P(u) = (2u^3 - 3u^2 + 1)P0 + (u^3 - 2u^2 + u)T0 + (-2u^3 + 3u^2)P1 + (u^3 - u^2)T1
    // where T0 and T1 are the tangents (out_tangent of a and in_tangent of b)
    
    const float u2 = u * u;
    const float u3 = u2 * u;
    
    const float h1 =  2.0f * u3 - 3.0f * u2 + 1.0f;
    const float h2 =         u3 - 2.0f * u2 + u;
    const float h3 = -2.0f * u3 + 3.0f * u2;
    const float h4 =         u3 -        u2;
    
    // Note: Tangents in NLEs are often stored as relative offsets.
    // If they are absolute vectors, they need to be scaled by duration?
    // In AE, spatial tangents are world-space vectors.
    
    return a.position * h1 + a.out_tangent * h2 + b.position * h3 + b.in_tangent * h4;
}

std::vector<math::Vector3> SpatialInterpolator::get_path(
    const std::vector<SpatialKeyframe>& keyframes,
    int segments_per_segment) {
    
    if (keyframes.size() < 2) return {};
    
    std::vector<math::Vector3> path;
    path.reserve((keyframes.size() - 1) * segments_per_segment + 1);
    
    for (std::size_t i = 0; i < keyframes.size() - 1; ++i) {
        const auto& a = keyframes[i];
        const auto& b = keyframes[i + 1];
        
        for (int s = 0; s < segments_per_segment; ++s) {
            float u = static_cast<float>(s) / static_cast<float>(segments_per_segment);
            float t = a.time + u * (b.time - a.time);
            path.push_back(evaluate(keyframes, t));
        }
    }
    path.push_back(keyframes.back().position);
    
    return path;
}

} // namespace tachyon::properties
