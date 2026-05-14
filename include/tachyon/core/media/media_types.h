#pragma once

namespace tachyon::media {

/**
 * @brief Types of assets that can be resolved and managed.
 */
enum class AssetType {
    VIDEO,
    AUDIO,
    IMAGE,
    MESH,
    FONT,
    PRESET,
    PROJECT,
    JSON,
    DATA,
    OTHER
};

/**
 * @brief Kind of asset reference in a scene.
 */
enum class AssetKind {
    Image,
    Video,
    Audio,
    Font,
    DataSource,
    Subtitle,
    Unknown
};

} // namespace tachyon::media
