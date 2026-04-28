#pragma once

#include "tachyon/tracker/algorithms/feature_tracker.h"
#include <vector>
#include <optional>

namespace tachyon::tracker {

class MotionEstimation {
public:
    static std::optional<std::vector<float>> estimate_affine(
        const std::vector<Point2f>& src,
        const std::vector<TrackPoint>& dst);

    static std::optional<std::vector<float>> estimate_homography(
        const std::vector<Point2f>& src,
        const std::vector<TrackPoint>& dst);

    static std::optional<std::vector<float>> estimate_homography_ransac(
        const std::vector<Point2f>& src,
        const std::vector<TrackPoint>& dst,
        int iterations,
        float threshold_px,
        float min_inlier_ratio,
        std::vector<bool>& inlier_mask);
};

} // namespace tachyon::tracker
