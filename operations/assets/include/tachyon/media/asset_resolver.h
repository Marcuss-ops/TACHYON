#pragma once

#include <optional>
#include "tachyon/media/asset_ref.h"
#include "tachyon/media/resolved_asset.h"

namespace tachyon {
namespace media {

/**
 * Interface for resolving asset references to runtime-loadable resources.
 *
 * Contract:
 * - AssetManager owns identity and metadata only.
 * - PathResolver owns filesystem/proxy path resolution.
 * - MediaManager owns runtime loading and media caches.
 * - ImageManager is internal to MediaManager; not a public asset API.
 */
class AssetResolver {
public:
    virtual ~AssetResolver() = default;

    /**
     * Resolves an asset reference to concrete paths for runtime loading.
     * Returns std::nullopt if the asset cannot be resolved.
     */
    virtual std::optional<ResolvedAsset> resolve(const AssetRef& ref) = 0;
};

} // namespace media
} // namespace tachyon
