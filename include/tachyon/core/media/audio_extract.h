#pragma once

#include "tachyon/core/media/media_error.h"
#include <filesystem>
#include <string>

namespace tachyon::core::media {

struct AudioExtractConfig {
    std::filesystem::path input_file;
    std::filesystem::path output_wav;
    int sample_rate{16000}; // Default for Whisper
    int channels{1};       // Default for Whisper
};

/**
 * @brief Contract for extracting audio from video files.
 * Implementation is provided by Backends (e.g., FFmpeg).
 */
class IAudioExtractor {
public:
    virtual ~IAudioExtractor() = default;
    
    /**
     * @brief Extracts audio from the input file to a WAV file.
     * @return MediaResult<void>
     */
    virtual MediaResult<void> extract(const AudioExtractConfig& config) = 0;
};

} // namespace tachyon::core::media
