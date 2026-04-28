#pragma once

#include "tachyon/tracker/algorithms/feature_tracker.h"
#include <vector>
#include <string>
#include <optional>
#include <array>

namespace tachyon::tracker {

/**
 * @brief PlanarTrack: a tracked planar surface over time.
 */
struct PlanarTrack {
    struct Frame {
        int frame_number{0};
        Point2f corners[4];
        float homography[9]{1,0,0, 0,1,0, 0,0,1};
        float confidence{1.0f};
        bool valid{false};
    };

    std::string name;
    std::vector<Frame> frames;
    
    // Compatibility fields for some algorithms
    std::vector<float> frame_times;
    std::vector<std::vector<float>> homography_per_frame;

    // Initial corner positions in the reference frame.
    Point2f initial_corners[4];
    int reference_frame{0};

    // Track metadata
    int width{0};   // source frame width (for normalisation)
    int height{0};  // source frame height

    // -----------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------

    const Frame* frame_at(int frame_number) const {
        for (const auto& f : frames)
            if (f.frame_number == frame_number) return &f;
        return nullptr;
    }

    [[nodiscard]] std::optional<std::array<Point2f, 4>> corner_pin_at(float time) const;
};

// RANSAC homography estimation
std::optional<std::vector<float>> estimate_homography_ransac(
    const std::vector<Point2f>& src,
    const std::vector<TrackPoint>& dst,
    int iterations = 100,
    float threshold = 5.0f);

} // namespace tachyon::tracker
