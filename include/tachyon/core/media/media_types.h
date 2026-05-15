#pragma once

#include <string>
#include <optional>
#include <cstdint>

namespace tachyon::media {

enum class AssetType {
    IMAGE,
    VIDEO,
    AUDIO,
    FONT,
    PROJECT,
    SUBTITLE,
    DATA_SOURCE,
    UNKNOWN
};

enum class AssetKind {
    Image,
    Video,
    Audio,
    Font,
    Subtitle,
    DataSource,
    Unknown
};

} // namespace tachyon::media

namespace tachyon::core::media {

struct VideoMetadata {
    std::string codec;
    int width{0};
    int height{0};
    double fps{0.0};
    double duration_seconds{0.0};
    std::int64_t bit_rate{0};
};

struct AudioMetadata {
    std::string codec;
    int sample_rate{0};
    int channels{0};
    int bit_depth{0};
    double duration_seconds{0.0};
    std::int64_t bit_rate{0};
};

struct FullMetadata {
    std::string path;
    std::string format;
    double duration_seconds{0.0};
    std::int64_t size_bytes{0};
    std::int64_t bit_rate{0};
    std::optional<VideoMetadata> video;
    std::optional<AudioMetadata> audio;
};

} // namespace tachyon::core::media
