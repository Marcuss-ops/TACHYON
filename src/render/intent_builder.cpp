#include "tachyon/render/intent_builder.h"
#include "tachyon/core/policy/engine_policy.h"

namespace tachyon::render {

namespace {

std::string normalize_primitive_id(const std::string& asset_id) {
    if (asset_id.rfind("primitive:", 0) == 0) {
        return asset_id;
    }

    if (asset_id == "quad" || asset_id == "primitive:quad") {
        return "primitive:quad";
    }
    if (asset_id == "circle" || asset_id == "primitive:circle") {
        return "primitive:circle";
    }
    if (asset_id == "triangle" || asset_id == "primitive:triangle") {
        return "primitive:triangle";
    }

    return asset_id;
}

} // namespace

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
        
        // 1. Resolve Mesh Asset
        if (layer.mesh_asset_id.has_value()) {
            const std::string& asset_id = *layer.mesh_asset_id;
            const std::string resolved_id = normalize_primitive_id(asset_id);

            resources.mesh_asset = resource_provider->get_mesh(resolved_id);

            if (!resources.mesh_asset) {
                layer_ok = false;
                result.diagnostics.add_error(
                    "render.intent.mesh_missing",
                    "Failed to resolve mesh asset: " + resolved_id,
                    layer.id);
            }
        }
        
        // 2. Resolve Texture / Image
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
        
        // 3. Resolve Deformations (3D / Morphing)
        if (layer.mesh_deform_id.has_value()) {
            const std::string& deform_id = *layer.mesh_deform_id;
            resources.mesh_deform = resource_provider->get_deform(deform_id);

            if (!resources.mesh_deform) {
                if (EngineValidationPolicy::instance().strict_assets) {
                    layer_ok = false;
                    result.diagnostics.add_error(
                        "render.intent.deform_missing",
                        "Strict Mode: Failed to resolve mesh deform: " + deform_id,
                        layer.id);
                } else {
                    result.diagnostics.add_warning(
                        "render.intent.deform_missing",
                        "Missing mesh deform: " + deform_id,
                        layer.id);
                }
            }
        }

        // Add to intent if we found something useful
        if (resources.mesh_asset || resources.texture_rgba || resources.mesh_deform) {
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
