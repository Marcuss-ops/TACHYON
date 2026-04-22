#include "frame_executor_internal.h"
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
    if (t >= track.keyframes.back().time) return track.keyframes.back().value;
    
    auto it = std::upper_bound(track.keyframes.begin(), track.keyframes.end(), t,
        [](double time, const CompiledKeyframe& kf) { return time < kf.time; });
    
    if (it == track.keyframes.begin() || it == track.keyframes.end()) return fallback;
    
    const auto& k0 = *(it - 1);
    const auto& k1 = *it;
    
    const double duration = k1.time - k0.time;
    if (duration <= 0.0) return k0.value;
    
    const double alpha = (t - k0.time) / duration;
    double eased_alpha = alpha;
    
    if (k0.easing == static_cast<uint32_t>(animation::EasingPreset::Custom)) {
        animation::CubicBezierEasing bezier{k0.cx1, k0.cy1, k0.cx2, k0.cy2};
        eased_alpha = bezier.evaluate(alpha);
    } else if (k0.easing != static_cast<uint32_t>(animation::EasingPreset::None)) {
        animation::CubicBezierEasing bezier;
        switch (static_cast<animation::EasingPreset>(k0.easing)) {
            case animation::EasingPreset::EaseIn:    bezier = animation::CubicBezierEasing::ease_in(); break;
            case animation::EasingPreset::EaseOut:   bezier = animation::CubicBezierEasing::ease_out(); break;
            case animation::EasingPreset::EaseInOut: bezier = animation::CubicBezierEasing::ease_in_out(); break;
            default: bezier = animation::CubicBezierEasing::linear(); break;
        }
        eased_alpha = bezier.evaluate(alpha);
    }
    
    return k0.value + eased_alpha * (k1.value - k0.value);
}

} // namespace tachyon
