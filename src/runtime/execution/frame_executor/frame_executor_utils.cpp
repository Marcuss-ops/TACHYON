#include "frame_executor_internal.h"
#include "tachyon/core/animation/easing.h"

#include <algorithm>
#include <cmath>

namespace tachyon {

std::uint64_t build_node_key(std::uint64_t global_key, const CompiledNode& node) {
    CacheKeyBuilder builder;
    builder.add_u64(global_key);
    builder.add_u32(node.node_id);
    builder.add_u32(node.topo_index);
    builder.add_u32(node.version);
    return builder.finish();
}

double sample_keyframed_value(const CompiledPropertyTrack& track, double fallback, double t) {
    if (track.keyframes.empty()) return fallback;
    if (t <= track.keyframes.front().time) return track.keyframes.front().value;
    
    // We allow extrapolation beyond the last keyframe to let springs settle.
    // If t > back.time, we'll use the last segment for evaluation.
    auto it = std::upper_bound(track.keyframes.begin(), track.keyframes.end(), t,
        [](double time, const CompiledKeyframe& kf) { return time < kf.time; });
    
    if (it == track.keyframes.begin()) return track.keyframes.front().value;
    
    // If it == end(), we are extrapolating past the last keyframe. 
    // We'll use the last two keyframes as the segment.
    // Note: requires at least two keyframes for valid extrapolation.
    const auto& k1 = (it == track.keyframes.end()) ? track.keyframes.back() : *it;
    const auto& k0 = (it == track.keyframes.end()) ? *(track.keyframes.end() - 2) : *(it - 1);
    
    const double duration = k1.time - k0.time;
    if (duration <= 0.0) return k0.value;

    const double alpha = (t - k0.time) / duration;
    const auto preset = static_cast<animation::EasingPreset>(k0.easing);
    const animation::CubicBezierEasing custom_bezier{k0.cx1, k0.cy1, k0.cx2, k0.cy2};
    const animation::SpringEasing spring_config{k0.spring_stiffness, k0.spring_damping, k0.spring_mass, 0.0};
    const double eased_alpha = animation::apply_easing(alpha, preset, custom_bezier, spring_config);
    
    return k0.value + eased_alpha * (k1.value - k0.value);
}

} // namespace tachyon
