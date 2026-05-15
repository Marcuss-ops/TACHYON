#pragma once

#include "tachyon/core/types/media_error.h"
#include <filesystem>
#include <string>

namespace tachyon::core::audio {

/**
 * @brief Core audio analysis engine (The "Nervous System" implementation).
 */
class AudioAnalyzer {
public:
    /**
     * @brief Transcribes an audio file using Whisper.
     * @param audio_path Path to the audio file.
     * @return Success message or MediaError.
     */
    static MediaResult<std::string> transcribe(const std::filesystem::path& audio_path);
};

} // namespace tachyon::core::audio
