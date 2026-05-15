#pragma once

#include "tachyon/core/media/media_error.h"
#include <filesystem>
#include <string>

namespace tachyon::core::media {

/**
 * @brief Utility for audio analysis and transcription.
 */
class AudioAnalyzer {
public:
    /**
     * @brief Transcribes an audio file using the configured backend (e.g. Whisper).
     * @param audio_path Path to the audio file.
     * @return Success message or MediaError.
     */
    static MediaResult<std::string> transcribe(const std::filesystem::path& audio_path);
};

} // namespace tachyon::core::media
