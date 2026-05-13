#pragma once

#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/media/resolved_asset.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace tachyon {

using AssetResolutionTable = std::map<std::string, media::ResolvedAsset>;

/**
 * Resolves all assets in a SceneSpec relative to a root directory.
 * Returns a ResolutionResult containing the table of resolved assets.
 */
ResolutionResult<AssetResolutionTable> resolve_assets(const SceneSpec& scene, const std::filesystem::path& root_dir);

struct AssetReport {
    struct Entry {
        std::string type;   // "image", "video", "audio", "font", "data_source", "subtitle", "unknown"
        std::filesystem::path path;
        bool exists{false};
        std::string error;
    };

    std::vector<Entry> entries;
    std::size_t image_count{0};
    std::size_t video_count{0};
    std::size_t audio_count{0};
    std::size_t font_count{0};
    std::size_t data_source_count{0};
    std::size_t subtitle_count{0};
    std::size_t unknown_count{0};
    bool all_present{false};
};

AssetReport build_asset_report(const SceneSpec& scene, const std::filesystem::path& root);

} // namespace tachyon
