#pragma once

#include "tachyon/core/media/video_concat.h"

namespace tachyon::backends::ffmpeg {

class FFmpegVideoConcat : public core::media::IVideoConcat {
public:
    core::MediaResult<void> concat_videos(const core::media::ConcatConfig& config) override;
};

} // namespace tachyon::backends::ffmpeg
