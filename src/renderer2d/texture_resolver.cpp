#include "tachyon/renderer2d/texture_resolver.h"

namespace tachyon::renderer2d {

media::AlphaMode TextureResolver::parse_alpha_mode(const std::optional<std::string>& mode) {
    if (!mode.has_value()) return media::AlphaMode::Straight;
    if (*mode == "premultiplied") return media::AlphaMode::Premultiplied;
    if (*mode == "ignore") return media::AlphaMode::Ignore;
    return media::AlphaMode::Straight;
}

void TextureResolver::resolve_textures(
    DrawList2D& draw_list, 
    const SceneSpec& scene,
    media::MediaManager& media_manager,
    double time_seconds) {
    (void)time_seconds; // Reserved for subtitle burn-in integration

    // Helper map to avoid repeated lookups
    // Maps asset_id -> SurfaceRGBA*
    std::unordered_map<std::string, const SurfaceRGBA*> resolved_cache;

    for (auto& command : draw_list.commands) {
        if (command.kind != DrawCommandKind::TexturedQuad || !command.textured_quad.has_value()) {
            continue;
        }

        auto& quad = *command.textured_quad;
        if (quad.texture.surface != nullptr || quad.texture.id.empty()) {
            continue;
        }

        const std::string& id = quad.texture.id;
        
        // Handle "image:" prefix
        if (id.compare(0, 6, "image:") == 0) {
            std::string layer_id = id.substr(6);
            
            // We need to find the asset associated with this layer
            // This is a bit roundabout because DrawListBuilder doesn't store asset info in handles
            // In a real project, we'd probably store asset_id in the handle during build
            
            bool found = false;
            for (const auto& comp : scene.compositions) {
                for (const auto& layer : comp.layers) {
                    if (layer.id == layer_id) {
                        // Found the layer, now find the asset
                        // Asset id might be the same as id/name or explicit
                        for (const auto& asset : scene.assets) {
                            if (asset.id == layer.id || asset.id == layer.name) {
                                const media::AlphaMode mode = parse_alpha_mode(asset.alpha_mode);
                                quad.texture.surface = media_manager.get_image(asset.path, mode);
                                found = true;
                                break;
                            }
                        }
                    }
                    if (found) break;
                }
                if (found) break;
            }
        }
    }
}

} // namespace tachyon::renderer2d
