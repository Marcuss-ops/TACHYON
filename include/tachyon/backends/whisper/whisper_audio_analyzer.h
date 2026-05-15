#pragma once

#include "tachyon/core/media/audio_analyzer.h"

namespace tachyon::backends::whisper {

class WhisperAudioAnalyzer : public core::media::IAudioAnalyzer {
public:
    core::MediaResult<std::string> transcribe(const std::filesystem::path& audio_path) override;
};

} // namespace tachyon::backends::whisper
