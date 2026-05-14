#pragma once

#include <string>
#include "tachyon/media/asset_manager.h"

namespace tachyon {
namespace media {

/**
 * Lightweight reference to an asset, used as input for resolution.
 * Does not carry runtime state or cached data.
 */
struct AssetRef {
    std::string id;
    std::string uri;
    AssetType type{AssetType::PROJECT};
};

} // namespace media
} // namespace tachyon
