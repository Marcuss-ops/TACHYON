#include "tachyon/camera_cut_contract.h"

#include <algorithm>
#include <iostream>

namespace tachyon {

void CameraCutTimeline::add_cut(const CameraCut& cut) {
    cuts_.push_back(cut);
}

std::optional<std::string> CameraCutTimeline::active_camera_at(double time) const {
    if (cuts_.empty()) return std::nullopt;

    for (const auto& cut : cuts_) {
        if (cut.start_seconds > time) break;
        if (cut.contains(time)) {
            return cut.camera_id;
        }
    }

    return std::nullopt;
}

bool CameraCutTimeline::validate(std::string& out_error) const {
    if (cuts_.empty()) return true;

    if (cuts_.front().camera_id.empty()) {
        out_error = "Camera cut has empty camera_id";
        return false;
    }

    for (std::size_t i = 0; i < cuts_.size(); ++i) {
        const auto& cut = cuts_[i];

        if (cut.camera_id.empty()) {
            out_error = "Camera cut has empty camera_id";
            return false;
        }
        if (cut.start_seconds > cut.end_seconds) {
            out_error = "Invalid camera cut: start time > end time";
            return false;
        }
        if (i > 0 && cut.start_seconds < cuts_[i - 1].start_seconds) {
            out_error = "Camera cuts are out of order";
            return false;
        }
        if (i > 0 && cuts_[i - 1].end_seconds > cut.start_seconds) {
            out_error = "Camera cuts overlap: cut at [" +
                       std::to_string(cuts_[i - 1].start_seconds) + ", " +
                       std::to_string(cuts_[i - 1].end_seconds) + ") overlaps with [" +
                       std::to_string(cut.start_seconds) + ", " +
                       std::to_string(cut.end_seconds) + ")";
            return false;
        }
    }

    return true;
}

} // namespace tachyon
