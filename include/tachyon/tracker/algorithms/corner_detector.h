#pragma once

#include "tachyon/tracker/algorithms/feature_tracker.h"
#include <vector>

namespace tachyon::tracker {

class CornerDetector {
public:
    static std::vector<Point2f> detect_harris(const GrayImage& frame, int max_features);
};

} // namespace tachyon::tracker
