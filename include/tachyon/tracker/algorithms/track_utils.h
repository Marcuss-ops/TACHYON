#pragma once

#include "tachyon/tracker/algorithms/feature_tracker.h"
#include <vector>

namespace tachyon::tracker {

class TrackUtils {
public:
    static std::vector<Point2f> smooth_track(
        const std::vector<Point2f>& raw, float alpha);
};

} // namespace tachyon::tracker
