#include "tachyon/camera_cut_contract.h"

#include <algorithm>
#include <iostream>

namespace tachyon {

void CameraCutTimeline::add_cut(const CameraCut& cut) {
    cuts_.push_back(cut);
    // Keep sorted by start time
    std::sort(cuts_.begin(), cuts_.end(),
             [](const CameraCut& a, const CameraCut& b) {
                 return a.start_seconds < b.start_seconds;
             });
}

std::optional<std::string> CameraCutTimeline::active_camera_at(double time) const {
    if (cuts_.empty()) return std::nullopt;

    // Find the last cut that contains (or precedes) the given time
    std::optional<std::string> last_camera;
    for (const auto& cut : cuts_) {
        if (cut.start_seconds > time) break;
        if (cut.contains(time)) {
            return cut.camera_id;
        }
        if (!cut.camera_id.empty()) {
            last_camera = cut.camera_id;
        }
    }

    // If time is after all cuts, hold the last camera
    return last_camera;
}

bool CameraCutTimeline::validate(std::string& out_error) const {
    if (cuts_.empty()) return true;

    // Check for overlaps and ordering
    for (std::size_t i = 0; i + 1 < cuts_.size(); ++i) {
        if (cuts_[i].end_seconds > cuts_[i + 1].start_seconds) {
            out_error = "Camera cuts overlap: cut at [" +
                       std::to_string(cuts_[i].start_seconds) + ", " +
                       std::to_string(cuts_[i].end_seconds) + ") overlaps with [" +
                       std::to_string(cuts_[i + 1].start_seconds) + ", " +
                       std::to_string(cuts_[i + 1].end_seconds) + ")";
            return false;
        }
        if (cuts_[i].start_seconds > cuts_[i].end_seconds) {
            out_error = "Invalid camera cut: start time > end time";
            return false;
        }
        if (cuts_[i].camera_id.empty()) {
            out_error = "Camera cut has empty camera_id";
            return false;
        }
    }

    if (!cuts_.empty() && cuts_.back().camera_id.empty()) {
        out_error = "Last camera cut has empty camera_id";
        return false;
    }

    return true;
}

} // namespace tachyon
