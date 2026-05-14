#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <string>
#include "tachyon/core/media/media_types.h"
#include <vector>

namespace tachyon::media {

// AssetKind moved to core/media/media_types.h

struct AssetReference {
    std::string id;
    std::string source;
    AssetKind kind;
    std::string owner_id; // composition or layer ID
};

/**
 * @brief Collects all asset references from a scene.
 */
std::vector<AssetReference> collect_asset_references(const SceneSpec& scene);

} // namespace tachyon::media
