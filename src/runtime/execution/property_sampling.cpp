#include "tachyon/runtime/execution/property_sampling.h"
#include "tachyon/core/animation/easing.h"
#include <algorithm>

namespace tachyon::runtime {

float sample_compiled_property_track(const tachyon::CompiledPropertyTrack& track, float time) {
    if (track.kind == tachyon::CompiledPropertyTrack::Kind::Constant) {
        return static_cast<float>(track.constant_value);
    }
    
    if (track.keyframes.empty()) {
        return static_cast<float>(track.constant_value);
    }

    if (time <= track.keyframes.front().time) {
        return static_cast<float>(track.keyframes.front().value);
    }
    if (time >= track.keyframes.back().time) {
        return static_cast<float>(track.keyframes.back().value);
    }

    auto it = std::upper_bound(track.keyframes.begin(), track.keyframes.end(), time,
        [](double t, const tachyon::CompiledKeyframe& kf) {
            return t < kf.time;
        });

    if (it == track.keyframes.begin()) return static_cast<float>(track.keyframes.front().value);

    const auto& k0 = *(it - 1);
    const auto& k1 = *it;

    double duration = k1.time - k0.time;
    double t = (duration > 1e-6) ? (time - k0.time) / duration : 0.0;

    // Apply easing
    tachyon::animation::CubicBezierEasing bezier{k0.cx1, k0.cy1, k0.cx2, k0.cy2};
    t = bezier.evaluate(t);

    return static_cast<float>(k0.value + (k1.value - k0.value) * t);
}

} // namespace tachyon::runtime
