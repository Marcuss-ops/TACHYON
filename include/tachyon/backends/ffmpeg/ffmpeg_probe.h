#pragma once

#include "tachyon/core/media/probe.h"

namespace tachyon::backends::ffmpeg {

class FFmpegProbe : public core::media::IMediaProbe {
public:
    core::MediaResult<core::media::FullMetadata> probe_file(const std::filesystem::path& path) override;
    core::MediaResult<core::media::FullMetadata> probe_full(const std::filesystem::path& path) override;
};

} // namespace tachyon::backends::ffmpeg
