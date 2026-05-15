#pragma once

#include "tachyon/core/media/media_interfaces.h"

namespace tachyon::backends::whisper {

class WhisperAudioAnalyzer : public core::media::IAudioAnalyzer {
public:
    core::MediaResult<std::vector<float>> analyze_waveform(const std::filesystem::path& path) override;
    core::MediaResult<std::string> transcribe(const std::filesystem::path& path) override;
};

} // namespace tachyon::backends::whisper
