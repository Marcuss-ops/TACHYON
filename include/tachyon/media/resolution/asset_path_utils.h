#pragma once
#include <filesystem>

namespace tachyon::media {

/**
 * Determines the logical root directory for resolving assets relative to a scene file.
 * Typically moves up one level if the scene is in a "scenes" or "project" subdirectory.
 */
std::filesystem::path scene_asset_root(const std::filesystem::path& scene_path);

} // namespace tachyon::media
