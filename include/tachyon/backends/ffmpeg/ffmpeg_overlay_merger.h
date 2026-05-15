#pragma once

#include "tachyon/core/media/overlay_merger.h"
#include <vector>
#include <string>

namespace tachyon::backends::ffmpeg {

class FFmpegOverlayMerger : public core::media::IOverlayMerger {
public:
    core::MediaResult<void> merge_overlays(const core::media::OverlayMergeConfig& config) override;

private:
    static std::string build_filter_complex(const core::media::OverlayMergeConfig& config);
    static std::string build_enable_condition(const core::media::OverlaySpec& overlay, const std::vector<std::pair<double, double>>& middle_clips);
};

} // namespace tachyon::backends::ffmpeg
