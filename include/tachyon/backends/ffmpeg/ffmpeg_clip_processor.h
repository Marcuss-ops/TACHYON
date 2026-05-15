#pragma once

#include "tachyon/core/media/clip_processor.h"
#include <string>

namespace tachyon::backends::ffmpeg {

class FFmpegClipProcessor : public core::media::IClipProcessor {
public:
    core::MediaResult<void> process_clip(const core::media::ClipProcessingConfig& config) override;

private:
    static std::string build_filter_complex(const core::media::ClipProcessingConfig& config, double duration);
};

} // namespace tachyon::backends::ffmpeg
