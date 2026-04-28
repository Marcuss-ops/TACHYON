#pragma once

#include "tachyon/tracker/algorithms/feature_tracker.h"
#include <string>
#include <vector>
#include <optional>

namespace tachyon::tracker {

/**
 * @brief A single time-stamped observation in a track.
 */
struct TrackSample {
    float time{0.0f};
    Point2f position{0.0f, 0.0f};
    float confidence{1.0f};
};

/**
 * @brief A 2D track — time-stamped positions plus optional solved transforms.
 * 
 * This is the canonical track data model. It stores observations and derived
 * transforms (affine or homography) per sample. All evaluation is deterministic
 * given the same samples.
 */
struct Track {
    std::string name;
    std::vector<TrackSample> samples;
    
    // Optional: solved 2x3 affine matrix (row-major: a,b,tx,c,d,ty) per sample
    std::optional<std::vector<std::vector<float>>> affine_per_sample;
    
    // Optional: solved 3x3 homography matrix (row-major) per sample
    std::optional<std::vector<std::vector<float>>> homography_per_sample;
    
    [[nodiscard]] std::optional<Point2f> position_at(float time) const;
    [[nodiscard]] std::optional<std::vector<float>> affine_at(float time) const;
    [[nodiscard]] std::optional<std::vector<float>> homography_at(float time) const;
};

} // namespace tachyon::tracker

