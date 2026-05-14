#pragma once

#include "tachyon/core/types/media_error.h"
#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <filesystem>

namespace tachyon::media {

/**
 * @brief Metadata for a video stream.
 */
struct VideoMetadata {
    std::string codec;
    int width{0};
    int height{0};
    double fps{0.0};
    double duration_seconds{0.0};
    std::int64_t bit_rate{0};
};

/**
 * @brief Metadata for an audio stream.
 */
struct AudioMetadata {
    std::string codec;
    int sample_rate{0};
    int channels{0};
    int bit_depth{0};
    double duration_seconds{0.0};
    std::int64_t bit_rate{0};
};

/**
 * @brief Unified metadata for a media file.
 */
struct FullMetadata {
    std::string path;
    std::string format;
    double duration_seconds{0.0};
    std::int64_t bit_rate{0};
    std::int64_t size_bytes{0};
    
    std::optional<VideoMetadata> video;
    std::optional<AudioMetadata> audio;
};

/**
 * @brief Utility for extracting metadata from media files.
 * Derived from ruststream-core/probe/mod.rs
 */
class MediaProbe {
public:
    /**
     * @brief Probes a media file and returns its metadata.
     * @param path Path to the media file.
     * @return A MediaResult containing FullMetadata or a MediaError.
     */
    static core::MediaResult<FullMetadata> probe_file(const std::filesystem::path& path);

private:
    /**
     * @brief Internal implementation that uses FFmpeg to probe the file.
     */
    static core::MediaResult<FullMetadata> probe_full(const std::filesystem::path& path);
};

} // namespace tachyon::media
