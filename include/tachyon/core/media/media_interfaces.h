#pragma once

#include "tachyon/core/media/media_types.h"
#include "tachyon/core/media/media_error.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <string>
#include <memory>
#include <vector>
#include <filesystem>

namespace tachyon::core::media {

/**
 * @brief Interface for probing media file metadata.
 */
class IMediaProbe {
public:
    virtual ~IMediaProbe() = default;
    virtual MediaResult<FullMetadata> probe_file(const std::filesystem::path& path) = 0;
    virtual MediaResult<FullMetadata> probe_full(const std::filesystem::path& path) = 0;
};

/**
 * @brief Interface for audio analysis (e.g. waveform, loudness, transcription).
 */
class IAudioAnalyzer {
public:
    virtual ~IAudioAnalyzer() = default;
    virtual MediaResult<std::vector<float>> analyze_waveform(const std::filesystem::path& path) = 0;
    virtual MediaResult<std::string> transcribe(const std::filesystem::path& path) = 0;
};

/**
 * @brief Interface for video encoding.
 */
class IVideoEncoder {
public:
    virtual ~IVideoEncoder() = default;
    virtual MediaResult<void> open(const std::filesystem::path& output_path, int width, int height, double fps) = 0;
    virtual MediaResult<void> write_frame(const renderer2d::SurfaceRGBA& surface) = 0;
    virtual void close() = 0;
};

} // namespace tachyon::core::media
