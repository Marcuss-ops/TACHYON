#include "tachyon/tracker/track.h"
#include <algorithm>
#include <cmath>

namespace tachyon::tracker {

std::optional<Point2f> Track::position_at(float time) const {
    if (samples.empty()) return std::nullopt;
    
    // Before first sample
    if (time <= samples.front().time) {
        return samples.front().position;
    }
    
    // After last sample
    if (time >= samples.back().time) {
        return samples.back().position;
    }
    
    // Binary search for segment
    auto it = std::lower_bound(
        samples.begin(), samples.end(), time,
        [](const TrackSample& s, float t) { return s.time < t; });
    
    if (it == samples.begin()) return it->position;
    
    const TrackSample& b = *it;
    const TrackSample& a = *(it - 1);
    
    if (a.time == b.time) return a.position;
    
    const float t = (time - a.time) / (b.time - a.time);
    return Point2f{
        a.position.x + t * (b.position.x - a.position.x),
        a.position.y + t * (b.position.y - a.position.y)
    };
}

std::optional<std::vector<float>> Track::affine_at(float time) const {
    if (!affine_per_sample.has_value() || affine_per_sample->empty()) {
        return std::nullopt;
    }
    if (samples.empty()) return std::nullopt;
    
    const auto& affines = *affine_per_sample;
    
    if (time <= samples.front().time) return affines.front();
    if (time >= samples.back().time) return affines.back();
    
    auto it = std::lower_bound(
        samples.begin(), samples.end(), time,
        [](const TrackSample& s, float t) { return s.time < t; });
    
    if (it == samples.begin()) return affines.front();
    
    const std::size_t idx = static_cast<std::size_t>(it - samples.begin());
    return affines[idx - 1]; // Hold interpolation for transforms
}

std::optional<std::vector<float>> Track::homography_at(float time) const {
    if (!homography_per_sample.has_value() || homography_per_sample->empty()) {
        return std::nullopt;
    }
    if (samples.empty()) return std::nullopt;
    
    const auto& homographies = *homography_per_sample;
    
    if (time <= samples.front().time) return homographies.front();
    if (time >= samples.back().time) return homographies.back();
    
    auto it = std::lower_bound(
        samples.begin(), samples.end(), time,
        [](const TrackSample& s, float t) { return s.time < t; });
    
    if (it == samples.begin()) return homographies.front();
    
    const std::size_t idx = static_cast<std::size_t>(it - samples.begin());
    return homographies[idx - 1]; // Hold interpolation for transforms
}

} // namespace tachyon::tracker
