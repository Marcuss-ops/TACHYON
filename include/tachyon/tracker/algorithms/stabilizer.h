#pragma once

#include "tachyon/tracker/algorithms/feature_tracker.h"
#include <vector>

namespace tachyon::tracker {

struct StabilizationResult {
    std::vector<std::vector<float>> transforms; // 3x3 homographies
};

class Stabilizer {
public:
    StabilizationResult stabilize(const TrackResult& tracking);
};

} // namespace tachyon::tracker

