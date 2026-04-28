#pragma once

#include "tachyon/tracker/algorithms/feature_tracker.h"
#include <vector>
#include <cstdint>

namespace tachyon::tracker {

class ImagePyramid {
public:
    using Levels = std::vector<std::vector<float>>;

    static Levels build(const GrayImage& img, int num_levels);
};

} // namespace tachyon::tracker
