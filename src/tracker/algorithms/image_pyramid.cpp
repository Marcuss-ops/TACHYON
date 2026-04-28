#include "tachyon/tracker/algorithms/image_pyramid.h"
#include <algorithm>

namespace tachyon::tracker {

ImagePyramid::Levels ImagePyramid::build(const GrayImage& img, int num_levels) {
    Levels pyr;
    uint32_t W = img.width, H = img.height;
    pyr.push_back(std::vector<float>(img.data, img.data + W * H));

    for (int level = 1; level < num_levels; ++level) {
        uint32_t nW = std::max(1u, W / 2), nH = std::max(1u, H / 2);
        const auto& prev = pyr.back();
        std::vector<float> cur(nW * nH, 0.0f);
        for (uint32_t y = 0; y < nH; ++y)
            for (uint32_t x = 0; x < nW; ++x) {
                // 2x2 box downsample
                uint32_t sx = x * 2, sy = y * 2;
                uint32_t sx1 = std::min(sx+1, W-1), sy1 = std::min(sy+1, H-1);
                cur[y * nW + x] = (prev[sy*W+sx] + prev[sy*W+sx1] +
                                    prev[sy1*W+sx] + prev[sy1*W+sx1]) * 0.25f;
            }
        pyr.push_back(std::move(cur));
        W = nW; H = nH;
    }
    return pyr;
}

} // namespace tachyon::tracker
