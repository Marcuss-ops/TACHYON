#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/media/resolution/asset_resolution.h"

#include <filesystem>
#include <vector>
#include <string>

namespace tachyon {

class AssetResolver {
public:
    // Add a directory to search for assets
    void add_search_path(const std::filesystem::path& path);

    // Resolve all assets in the scene spec
    // Returns a table mapping asset IDs to resolved paths
    AssetResolutionTable resolve_all(const SceneSpec& scene);

    // Try to resolve a single asset path
    // Returns the resolved absolute path, or empty if not found
    std::filesystem::path resolve(const std::filesystem::path& asset_path) const;

private:
    std::vector<std::filesystem::path> search_paths_;
};

} // namespace tachyon
