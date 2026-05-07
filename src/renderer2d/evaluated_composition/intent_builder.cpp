#include "tachyon/renderer2d/evaluated_composition/intent_builder.h"
#include "tachyon/media/management/media_manager.h"
#include <iostream>

namespace tachyon::renderer2d {

RenderIntent build_render_intent(
    const scene::EvaluatedCompositionState& state,
    const RenderContext2D& context) {
    
    RenderIntent intent;
    
    for (const auto& layer : state.layers) {
        if (!layer.active || !layer.visible) continue;
        
        RenderIntent::LayerResources resources;
        bool has_resources = false;
        
        // Resolve Mesh Asset
        if (layer.mesh_asset_id.has_value()) {
            const std::string& asset_id = *layer.mesh_asset_id;
            if (asset_id == "quad") {
                // The renderer should handle "quad" internally or we resolve it here
                // For now, let's assume the media manager or a primitive factory can provide it
                if (context.media_manager) {
                    resources.mesh_asset = context.media_manager->get_mesh_asset("primitive:quad");
                }
            } else if (context.media_manager) {
                resources.mesh_asset = context.media_manager->get_mesh_asset(asset_id);
            }
            
            if (resources.mesh_asset) {
                has_resources = true;
            }
        }
        
        // Resolve Texture / Image
        if (layer.texture_asset_id.has_value() && context.media_manager) {
            // resources.texture_rgba = ... (depends on how textures are managed)
            // For now, many textures are managed via TextureHandle in draw commands,
            // but for raw access during composition we might need this.
        }

        if (has_resources) {
            intent.layer_resources[layer.id] = std::move(resources);
        }
    }
    
    return intent;
}

} // namespace tachyon::renderer2d
