#pragma once

#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

#include <filesystem>
#include <map>
#include <string>

namespace tachyon {

struct ResolvedAsset {
    std::string asset_id;
    std::string type;
    std::filesystem::path absolute_path;
    bool exists{false};
};

using AssetResolutionTable = std::map<std::string, ResolvedAsset>;

/**
 * Resolves all assets in a SceneSpec relative to a root directory.
 * Returns a ResolutionResult containing the table of resolved assets.
 */
ResolutionResult<AssetResolutionTable> resolve_assets(const SceneSpec& scene, const std::filesystem::path& root_dir);

struct AssetReport {
    struct Entry {
        std::string type;   // "image", "video", "audio", "font"
        std::filesystem::path path;
        bool exists{false};
        std::string error;
    };

    std::vector<Entry> entries;
    std::size_t image_count{0};
    std::size_t video_count{0};
    std::size_t audio_count{0};
    std::size_t font_count{0};
    bool all_present{false};
};

AssetReport build_asset_report(const SceneSpec& scene, const std::filesystem::path& root);

} // namespace tachyon
