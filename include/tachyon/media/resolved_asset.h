#pragma once

#include <filesystem>
#include <string>
#include "tachyon/media/asset_manager.h"

namespace tachyon {
namespace media {

/**
 * Result of resolving an asset reference to concrete filesystem paths.
 * Produced by AssetResolver, consumed by MediaManager for runtime loading.
 */
struct ResolvedAsset {
    std::string id;
    std::filesystem::path source_path;
    std::filesystem::path runtime_path;
    AssetType type{AssetType::PROJECT};
    bool uses_proxy{false};
};

} // namespace media
} // namespace tachyon
