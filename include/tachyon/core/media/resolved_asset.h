#pragma once

#include <filesystem>
#include <string>
#include "tachyon/core/media/media_types.h"

namespace tachyon::media {

/**
 * Result of resolving an asset reference to concrete filesystem paths.
 * Produced by AssetResolver, consumed by MediaManager for runtime loading.
 */
struct ResolvedAsset {
    std::string id;                 // Asset ID or original spec
    std::string type_name;          // "image", "video", "audio", "font"
    std::filesystem::path source_path;
    std::filesystem::path runtime_path;
    AssetType type{AssetType::PROJECT};
    bool exists{false};
    bool uses_proxy{false};
};

} // namespace tachyon::media
