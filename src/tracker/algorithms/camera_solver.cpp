#include "tachyon/tracker/algorithms/camera_solver.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace tachyon::tracker {

namespace {

struct TimeBucket {
    float time{0.0f};
    math::Vector2 sum{0.0f, 0.0f};
    int count{0};
};

std::vector<float> collect_unique_times(const std::vector<Track>& tracks) {
    std::vector<float> times;
    for (const auto& track : tracks) {
        for (const auto& sample : track.samples) {
            times.push_back(sample.time);
        }
    }
    std::sort(times.begin(), times.end());
    times.erase(std::unique(times.begin(), times.end(), [](float a, float b) {
        return std::abs(a - b) < 1.0e-6f;
    }), times.end());
    return times;
}

math::Vector2 average_position_at_time(const std::vector<Track>& tracks, float time) {
    math::Vector2 sum{0.0f, 0.0f};
    int count = 0;

    for (const auto& track : tracks) {
        for (const auto& sample : track.samples) {
            if (std::abs(sample.time - time) < 1.0e-6f && sample.confidence > 0.0f) {
                sum.x += sample.position.x;
                sum.y += sample.position.y;
                ++count;
                break;
            }
        }
    }

    if (count == 0) {
        return {0.0f, 0.0f};
    }

    return {sum.x / static_cast<float>(count), sum.y / static_cast<float>(count)};
}

float pixel_to_world_scale(const CameraSolver::Config& config) {
    const float frame_extent = static_cast<float>(std::max(config.frame_width, config.frame_height));
    if (frame_extent <= 0.0f) {
        return 1.0f;
    }
    return 1.0f / frame_extent;
}

} // namespace

CameraTrack CameraSolver::solve(const std::vector<Track>& tracks, const Config& config) const {
    CameraTrack out;

    const std::vector<float> times = collect_unique_times(tracks);
    if (times.empty()) {
        return out;
    }

    out.keyframes.reserve(times.size());
    const float scale = pixel_to_world_scale(config);
    const float focal = config.initial_focal_length_mm.value_or(35.0f);

    math::Vector3 accumulated_position = math::Vector3::zero();
    math::Vector2 prev_avg = average_position_at_time(tracks, times.front());

    out.keyframes.push_back(CameraTrackKeyframe{
        times.front(),
        math::Vector3::zero(),
        math::Quaternion::identity(),
        focal,
        0.0f
    });

    for (std::size_t i = 1; i < times.size(); ++i) {
        const float time = times[i];
        const math::Vector2 avg = average_position_at_time(tracks, time);
        const math::Vector2 delta = avg - prev_avg;

        // A rightward motion in screen space corresponds to a leftward camera move.
        // We invert the delta so the solved camera moves in the intuitive direction
        // expected by the tracker tests and downstream authoring tools.
        accumulated_position.x += -delta.x * scale;
        accumulated_position.y += -delta.y * scale;

        out.keyframes.push_back(CameraTrackKeyframe{
            time,
            accumulated_position,
            math::Quaternion::identity(),
            focal,
            0.0f
        });

        prev_avg = avg;
    }

    return out;
}

} // namespace tachyon::tracker

