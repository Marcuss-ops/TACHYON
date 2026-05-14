#pragma once

#include <string>

namespace tachyon::media {

/**
 * @brief Represents the current availability state of a media asset.
 */
enum class MediaAssetState {
    Offline,
    Online,
    ProxyPending,
    ProxyAvailable,
    OriginalAvailable,
    DecodePending
};

/**
 * @brief Describes the technical metadata of a media source.
 */
struct MediaSourceDescriptor {
    std::string original_path;
    std::string proxy_path;
    std::string metadata; // JSON or opaque string
    double duration{0.0};
    double fps{24.0};
    std::string color_space{"sRGB"};
};

/**
 * @brief Represents a single media asset in the project.
 */
class MediaAsset {
public:
    MediaAsset(std::string id, MediaSourceDescriptor descriptor)
        : m_id(std::move(id)), m_descriptor(std::move(descriptor)) {}

    [[nodiscard]] const std::string& id() const { return m_id; }
    [[nodiscard]] MediaAssetState state() const { return m_state; }
    [[nodiscard]] const MediaSourceDescriptor& descriptor() const { return m_descriptor; }

    void set_state(MediaAssetState state) { m_state = state; }

private:
    std::string m_id;
    MediaAssetState m_state{MediaAssetState::Offline};
    MediaSourceDescriptor m_descriptor;
};

} // namespace tachyon::media
