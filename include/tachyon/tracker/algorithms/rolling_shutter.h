#pragma once

#include "tachyon/tracker/algorithms/feature_tracker.h"
#include <vector>

namespace tachyon::tracker {

struct RollingShutterParams {
    float readout_time_ms{30.0f}; // Readout duration in milliseconds
    int scan_direction{1};       // 1 = top-to-bottom, -1 = bottom-to-top
    float frame_duration_ms{41.666f}; // 24fps default
};

/**
 * Apply rolling shutter correction to a sequence of frames.
 * @param frames The sequence of greyscale frames.
 * @param homographies 3x3 homographies for each frame (from FeatureTracker).
 * @param params Rolling shutter parameters.
 */
void apply_rolling_shutter_correction(
    const std::vector<std::vector<float>>& homographies,
    const RollingShutterParams& params,
    std::vector<std::vector<float>>& frame_data,
    uint32_t width,
    uint32_t height);

} // namespace tachyon::tracker

