#include "tachyon/camera_cut_contract.h"

#include <algorithm>
#include <iostream>

namespace tachyon {

void CameraCutTimeline::add_cut(const CameraCut& cut) {
    cuts_.push_back(cut);
}

std::optional<std::string> CameraCutTimeline::active_camera_at(double time) const {
    if (cuts_.empty()) return std::nullopt;

    std::vector<CameraCut> sorted = cuts_;
    std::sort(sorted.begin(), sorted.end(),
             [](const CameraCut& a, const CameraCut& b) {
                 return a.start_seconds < b.start_seconds;
             });

    std::optional<std::string> last_camera;
    const CameraCut* previous = nullptr;
    for (const auto& cut : sorted) {
        if (cut.start_seconds > time) {
            if (previous && time > previous->end_seconds && last_camera.has_value()) {
                return last_camera;
            }
            return std::nullopt;
        }
        if (cut.contains(time)) {
            return cut.camera_id;
        }
        if (!cut.camera_id.empty()) {
            last_camera = cut.camera_id;
        }
        previous = &cut;
    }

    return std::nullopt;
}

bool CameraCutTimeline::validate(std::string& out_error) const {
    if (cuts_.empty()) return true;

    // Check ordering, overlaps, and empty IDs in stored order.
    for (std::size_t i = 0; i < cuts_.size(); ++i) {
        const auto& cut = cuts_[i];
        if (cut.start_seconds > cut.end_seconds) {
            out_error = "Invalid camera cut: start time > end time";
            return false;
        }
        if (cut.camera_id.empty()) {
            out_error = "Camera cut has empty camera_id";
            return false;
        }

        if (i + 1 < cuts_.size()) {
            const auto& next = cuts_[i + 1];
            if (next.start_seconds < cut.start_seconds) {
                out_error = "Camera cuts out of order";
                return false;
            }
            if (cut.end_seconds > next.start_seconds) {
                out_error = "Camera cuts overlap: cut at [" +
                           std::to_string(cut.start_seconds) + ", " +
                           std::to_string(cut.end_seconds) + ") overlaps with [" +
                           std::to_string(next.start_seconds) + ", " +
                           std::to_string(next.end_seconds) + ")";
                return false;
            }
        }
    }

    return true;
}

} // namespace tachyon
