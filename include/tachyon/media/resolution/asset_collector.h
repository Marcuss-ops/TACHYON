#pragma once
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <string>
#include <vector>
#include <filesystem>

namespace tachyon::media {

enum class AssetKind {
    Image,
    Video,
    Audio,
    Font,
    DataSource,
    Subtitle,
    Unknown
};

struct AssetReference {
    AssetKind kind;
    std::string id;        // May be empty for untracked assets (e.g. data sources)
    std::string source;    // Original path/ID from the spec
    std::string owner_id;  // ID of the layer/composition/manifest containing this reference
};

/**
 * @brief Traverses the entire SceneSpec and collects every asset reference.
 * This is the single source of truth for "what files does this scene need?".
 */
std::vector<AssetReference> collect_asset_references(const SceneSpec& scene);

} // namespace tachyon::media
