#pragma once

#include "tachyon/core/media/audio_extract.h"

namespace tachyon::backends::ffmpeg {

class FFmpegAudioExtractor : public core::media::IAudioExtractor {
public:
    core::MediaResult<void> extract(const core::media::AudioExtractConfig& config) override;
};

} // namespace tachyon::backends::ffmpeg
