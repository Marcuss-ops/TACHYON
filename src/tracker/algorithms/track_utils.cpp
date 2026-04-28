#include "tachyon/tracker/algorithms/track_utils.h"

namespace tachyon::tracker {

std::vector<Point2f> TrackUtils::smooth_track(
    const std::vector<Point2f>& raw, float alpha) {

    if (raw.empty()) return {};
    std::vector<Point2f> out;
    out.reserve(raw.size());
    out.push_back(raw[0]);
    for (size_t i = 1; i < raw.size(); ++i) {
        float sx = alpha * raw[i].x + (1 - alpha) * out.back().x;
        float sy = alpha * raw[i].y + (1 - alpha) * out.back().y;
        out.push_back({sx, sy});
    }
    return out;
}

} // namespace tachyon::tracker
