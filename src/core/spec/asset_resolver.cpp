#include "tachyon/core/spec/asset_resolver.h"
#include <algorithm>

namespace tachyon {

void AssetResolver::add_search_path(const std::filesystem::path& path) {
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
        search_paths_.push_back(path);
    }
}

std::filesystem::path AssetResolver::resolve(const std::filesystem::path& asset_path) const {
    // If already absolute and exists, return it
    if (asset_path.is_absolute()) {
        if (std::filesystem::exists(asset_path)) {
            return asset_path;
        }
        return {};
    }

    // Search in all search paths
    for (const auto& search_path : search_paths_) {
        std::filesystem::path full_path = search_path / asset_path;
        if (std::filesystem::exists(full_path)) {
            return full_path;
        }
    }

    return {};
}

AssetResolutionTable AssetResolver::resolve_all(const SceneSpec& scene) {
    AssetResolutionTable table;

    // Add scene directory and common asset directories as search paths
    // (if not already added by the caller)

    // Resolve each asset in the scene
    for (const auto& asset : scene.assets) {
        ResolvedAsset resolved;
        resolved.asset_id = asset.id;
        resolved.type = asset.type;

        // Try to resolve the asset path
        std::filesystem::path asset_path = asset.path;
        std::filesystem::path resolved_path = resolve(asset_path);

        if (!resolved_path.empty()) {
            resolved.absolute_path = resolved_path;
            resolved.exists = true;
        } else {
            // Try with common extensions if no extension
            if (asset_path.extension().empty()) {
                // Try common image extensions
                for (const auto& ext : {".png", ".jpg", ".jpeg", ".webp", ".bmp"}) {
                    resolved_path = resolve(asset_path.string() + ext);
                    if (!resolved_path.empty()) {
                        resolved.absolute_path = resolved_path;
                        resolved.exists = true;
                        break;
                    }
                }
            }
        }

        table[asset.id] = resolved;
    }

    return table;
}

} // namespace tachyon
