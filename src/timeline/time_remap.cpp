#include "tachyon/timeline/time_remap.h"
#include <algorithm>

namespace tachyon::timeline {

float evaluate_source_time(const TimeRemapCurve& curve, float dest_time) {
    if (curve.keyframes.empty()) {
        return dest_time;
    }

    // Binary search for the segment
    auto it = std::lower_bound(curve.keyframes.begin(), curve.keyframes.end(), std::make_pair(0.0f, dest_time),
        [](const std::pair<float, float>& a, const std::pair<float, float>& b) {
            return a.second < b.second;
        });

    if (it == curve.keyframes.begin()) {
        return it->first;
    }
    if (it == curve.keyframes.end()) {
        return curve.keyframes.back().first;
    }

    auto prev = std::prev(it);
    
    // Linear interpolation between (prev_src, prev_dest) and (curr_src, curr_dest)
    float t = (dest_time - prev->second) / (it->second - prev->second);
    
    if (curve.mode == TimeRemapMode::Hold) {
        return prev->first;
    }
    
    return prev->first + t * (it->first - prev->first);
}

FrameBlendResult evaluate_frame_blend(
    const TimeRemapCurve& curve, 
    float dest_time, 
    float frame_duration) {
    
    const float source_time = evaluate_source_time(curve, dest_time);
    
    // Calculate nearest integer frame boundaries in source time space
    const double frame_a = std::floor(static_cast<double>(source_time) / static_cast<double>(frame_duration)) * static_cast<double>(frame_duration);
    const double frame_b = frame_a + static_cast<double>(frame_duration);
    
    const float factor = (frame_duration > 1e-6f) 
        ? static_cast<float>((static_cast<double>(source_time) - frame_a) / static_cast<double>(frame_duration))
        : 0.0f;
    
    return { frame_a, frame_b, std::clamp(factor, 0.0f, 1.0f) };
}

} // namespace tachyon::timeline
