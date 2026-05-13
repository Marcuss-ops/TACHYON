#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

namespace tachyon::render {

/**
 * @brief RenderIntent captures renderer-specific resources associated with evaluated scene layers.
 * This decouples the scene evaluation (Core) from rendering resources (Renderer).
 */
struct RenderIntent {
    struct LayerResources {
        std::shared_ptr<std::uint8_t[]> texture_rgba;
        
        // Potential for more neutral resources (effects, overlays)
    };

    // Mapping from layer_id to its renderer-specific resources
    std::unordered_map<std::string, LayerResources> layer_resources;

    // Environment map (Background context)
    std::optional<std::string> environment_map_id;
};

} // namespace tachyon::render
