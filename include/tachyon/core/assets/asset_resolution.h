#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/media/media_types.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/core/media/resolved_asset.h"
#include "tachyon/api.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>
#include <map>

namespace tachyon::media { class IAssetResolver; }

namespace tachyon::core::assets {

using AssetResolutionTable = std::map<std::string, ::tachyon::media::ResolvedAsset>;

/**
 * @brief Resolves all assets in a SceneSpec using the provided resolver.
 */
TACHYON_API ResolutionResult<AssetResolutionTable> resolve_assets(
    const SceneSpec& scene, 
    const ::tachyon::media::IAssetResolver& resolver);

/**
 * @brief Detailed report of asset presence and types.
 */
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

/**
 * @brief Builds a descriptive report of all assets required by the scene.
 */
TACHYON_API AssetReport build_asset_report(
    const SceneSpec& scene, 
    const ::tachyon::media::IAssetResolver& resolver);

} // namespace tachyon::core::assets
