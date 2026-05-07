#pragma once

#include "tachyon/media/loading/mesh_asset.h"
#include <memory>
#include <string>
#include <unordered_map>

namespace tachyon::renderer2d {

struct DeformMesh;

/**
 * @brief RenderIntent captures renderer-specific resources associated with evaluated scene layers.
 * This decouples the scene evaluation (Core) from rendering resources (Renderer).
 */
struct RenderIntent {
    struct LayerResources {
        std::shared_ptr<::tachyon::media::MeshAsset> mesh_asset;
        std::shared_ptr<const DeformMesh> mesh_deform;
        std::shared_ptr<std::uint8_t[]> texture_rgba;
    };

    // Mapping from layer_id to its renderer-specific resources
    std::unordered_map<std::string, LayerResources> layer_resources;
    
    // Environment map (skeletons / IBL)
    const void* environment_map{nullptr};
};

} // namespace tachyon::renderer2d
