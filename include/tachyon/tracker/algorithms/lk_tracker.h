#pragma once

#include "tachyon/tracker/algorithms/feature_tracker.h"
#include <vector>
#include <cstdint>

namespace tachyon::tracker {

class LKTracker {
public:
    struct Params {
        int   patch_radius{7};
        int   lk_iterations{20};
        float lk_epsilon{0.03f};
        float min_confidence{0.1f};
    };

    static TrackPoint track_point(
        const std::vector<float>& prev_level,
        const std::vector<float>& next_level,
        uint32_t w, uint32_t h,
        Point2f  prev_pos,
        Point2f  guess,
        const Params& params);
};

} // namespace tachyon::tracker
