#pragma once

#include "tachyon/core/media/media_error.h"
#include <filesystem>
#include <string>

namespace tachyon::core::media {

/**
 * @brief Contract for audio analysis and transcription.
 */
class IAudioAnalyzer {
public:
    virtual ~IAudioAnalyzer() = default;

    /**
     * @brief Transcribes an audio file using a backend (e.g. Whisper).
     * @param audio_path Path to the audio file.
     * @return Success result containing transcription or MediaError.
     */
    virtual MediaResult<std::string> transcribe(const std::filesystem::path& audio_path) = 0;
};

} // namespace tachyon::core::media
