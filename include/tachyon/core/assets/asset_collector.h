#pragma once

#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/media/media_types.h"
#include "tachyon/api.h"
#include <string>
#include <vector>

namespace tachyon::core::assets {

/**
 * @brief Represents a reference to an asset found within a scene.
 */
struct AssetReference {
    std::string id;
    std::string source;
    ::tachyon::media::AssetKind kind;
    std::string owner_id; // ID of the composition or layer that owns this reference
};

/**
 * @brief Collects all asset references from a SceneSpec.
 * This is a semantic analysis of the scene requirements.
 */
TACHYON_API std::vector<AssetReference> collect_asset_references(const SceneSpec& scene);

} // namespace tachyon::core::assets
