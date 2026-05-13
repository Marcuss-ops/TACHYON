#include "tachyon/render/intent_builder.h"
#include "tachyon/core/policy/engine_policy.h"

namespace tachyon::render {

RenderIntentBuildResult build_render_intent(
    const scene::EvaluatedCompositionState& state,
    IResourceProvider* resource_provider) {
    
    RenderIntentBuildResult result;
    
    if (!resource_provider) {
        result.status = IntentBuildStatus::InternalError;
        result.diagnostics.add_error(
            "render.intent.null_resource_provider",
            "Null resource provider passed to build_render_intent.");
        return result;
    }
    
    for (const auto& layer : state.layers) {
        if (!layer.active || !layer.visible) continue;
        
        RenderIntent::LayerResources resources;
        bool layer_ok = true;
        
        // Resolve Texture / Image
        if (layer.texture_asset_id.has_value()) {
            const std::string& asset_id = *layer.texture_asset_id;
            resources.texture_rgba = resource_provider->get_texture_rgba(asset_id);

            if (!resources.texture_rgba && !asset_id.empty()) {
                if (EngineValidationPolicy::instance().strict_assets) {
                    layer_ok = false;
                    result.diagnostics.add_error(
                        "render.intent.texture_missing",
                        "Strict Mode: Failed to resolve texture asset: " + asset_id,
                        layer.id);
                } else {
                    result.diagnostics.add_warning(
                        "render.intent.texture_missing",
                        "Missing texture asset: " + asset_id,
                        layer.id);
                }
            }
        }
        
        // Add to intent if we found something useful
        if (resources.texture_rgba) {
            result.intent.layer_resources[layer.id] = std::move(resources);
        }
        
        if (!layer_ok) {
            result.status = IntentBuildStatus::PartialSuccess;
        }
    }
    
    // Final environment map resolution
    if (state.environment_map_id.has_value()) {
        result.intent.environment_map_id = state.environment_map_id;
    }
    
    if (result.has_errors()) {
        if (EngineValidationPolicy::instance().strict_assets) {
            result.status = IntentBuildStatus::AssetResolutionError;
        }
    }
    
    return result;
}

} // namespace tachyon::render
