#pragma once

#include "tachyon/media/loading/mesh_asset.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace tachyon::render {

struct IDeformMesh {
    virtual ~IDeformMesh() = default;
};

/**
 * @brief RenderIntent captures renderer-specific resources associated with evaluated scene layers.
 * This decouples the scene evaluation (Core) from rendering resources (Renderer).
 * 
 * It lives in a neutral namespace [tachyon::render] to allow SceneEval to build
 * without linking specific renderers.
 */
struct RenderIntent {
    struct LayerResources {
        std::shared_ptr<::tachyon::media::MeshAsset> mesh_asset;
        std::shared_ptr<const IDeformMesh> mesh_deform;
        std::shared_ptr<std::uint8_t[]> texture_rgba;
        
        // Potential for more neutral resources (skeletons, materials)
    };

    // Mapping from layer_id to its renderer-specific resources
    std::unordered_map<std::string, LayerResources> layer_resources;

    // Environment map (skeletons / IBL / Background context)
    // Replaced void* with a robust optional ID for product-grade hardening.
    std::optional<std::string> environment_map_id;
};

} // namespace tachyon::render
