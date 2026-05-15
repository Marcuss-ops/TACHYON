#pragma once

#include "tachyon/core/types/media_error.h"
#include <filesystem>
#include <string>

namespace tachyon::media {

class AudioAnalyzer {
public:
    /**
     * @brief Transcribes an audio file using Whisper.
     * @param audio_path Path to the audio file.
     * @return Success message or MediaError.
     */
    static core::MediaResult<std::string> transcribe(const std::filesystem::path& audio_path);
};

} // namespace tachyon::media
