#include "tachyon/tracker/algorithms/stabilizer.h"
#include <algorithm>
#include <vector>

namespace tachyon::tracker {

StabilizationResult Stabilizer::stabilize(const TrackResult& tracking) {
    StabilizationResult result;
    if (tracking.frames.empty()) return result;

    const size_t n_frames = tracking.frames.size();
    result.transforms.resize(n_frames);

    // Initial pass: use raw homographies from tracking
    for (size_t i = 0; i < n_frames; ++i) {
        if (i < tracking.homography_per_frame.size() && !tracking.homography_per_frame[i].empty()) {
            result.transforms[i] = tracking.homography_per_frame[i];
        } else {
            // Identity
            result.transforms[i] = {1, 0, 0, 0, 1, 0, 0, 0, 1};
        }
    }

    // Smoothing pass (e.g. moving average of homographies)
    // This is where 'stabilization' actually happens
    // For now, it just returns the raw tracking motion
    
    return result;
}

} // namespace tachyon::tracker

