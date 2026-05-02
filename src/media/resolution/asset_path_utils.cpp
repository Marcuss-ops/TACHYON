#include "tachyon/media/resolution/asset_path_utils.h"
#include <string>

namespace tachyon::media {

std::filesystem::path scene_asset_root(const std::filesystem::path& scene_path) {
    const auto scene_dir = scene_path.parent_path();
    if (scene_dir.empty()) return scene_dir;
    const auto folder_name = scene_dir.filename().string();
    if ((folder_name == "scenes" || folder_name == "project") && !scene_dir.parent_path().empty()) {
        return scene_dir.parent_path();
    }
    return scene_dir;
}

std::filesystem::path resolve_relative_to_root(
    const std::filesystem::path& value,
    const std::filesystem::path& root
) {
    if (value.is_absolute()) {
        return value;
    }
    return std::filesystem::absolute(root / value);
}

std::filesystem::path asset_source_or_path(const AssetSpec& asset) {
    if (!asset.source.empty()) {
        return std::filesystem::path(asset.source);
    }
    return std::filesystem::path(asset.path);
}

} // namespace tachyon::media
