#pragma once
#include <filesystem>
#include "tachyon/core/spec/schema/objects/scene_spec.h"

namespace tachyon::media {

/**
 * Determines the logical root directory for resolving assets relative to a scene file.
 * Typically moves up one level if the scene is in a "scenes" or "project" subdirectory.
 */
std::filesystem::path scene_asset_root(const std::filesystem::path& scene_path);

/**
 * Resolves a path relative to a root directory.
 * If the path is already absolute, it is returned as-is.
 */
std::filesystem::path resolve_relative_to_root(
    const std::filesystem::path& value,
    const std::filesystem::path& root
);

/**
 * Returns the effective path for an asset.
 * Prefers asset.source if present, otherwise falls back to asset.path.
 */
std::filesystem::path asset_source_or_path(const AssetSpec& asset);

} // namespace tachyon::media
