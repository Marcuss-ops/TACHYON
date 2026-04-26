#include "tachyon/renderer2d/resource/texture_resolver.h"

#include <iostream>
#include <memory>
#include <vector>

namespace tachyon::renderer2d {

const ::tachyon::text::FontRegistry* get_default_font_registry() {
    static std::once_flag once;
    static ::tachyon::text::FontRegistry* default_registry = nullptr;
    
    std::call_once(once, []() {
        default_registry = new ::tachyon::text::FontRegistry();
        
        const std::vector<std::filesystem::path> candidates = {
            // Repo-local fonts first so the renderer can use assets shipped with the project.
            std::filesystem::path("fonts/SegoeUI.ttf"),
            std::filesystem::path("fonts/Arial.ttf"),
            std::filesystem::path("fonts/Tahoma.ttf"),
            std::filesystem::path("fonts/NotoColorEmoji.ttf"),
            // Windows paths
            std::filesystem::path(R"(C:\Windows\Fonts\arial.ttf)"),
            std::filesystem::path(R"(C:\Windows\Fonts\Arial.ttf)"),
            std::filesystem::path(R"(C:\Windows\Fonts\arialuni.ttf)"),
            std::filesystem::path(R"(C:\Windows\Fonts\LiberationSans-Regular.ttf)"),
            std::filesystem::path(R"(C:\Windows\Fonts\DejaVuSans.ttf)"),
            // Linux paths
            std::filesystem::path("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"),
            std::filesystem::path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"),
            std::filesystem::path("/usr/share/fonts/truetype/freefont/FreeSans.ttf"),
            std::filesystem::path("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf"),
            std::filesystem::path("/usr/share/fonts/TTF/DejaVuSans.ttf"),
            std::filesystem::path("/usr/share/fonts/TTF/LiberationSans-Regular.ttf")
        };
        
        for (const auto& candidate : candidates) {
            if (std::filesystem::exists(candidate)) {
                if (default_registry->load_ttf("Default", candidate, 48U)) {
                    default_registry->set_default("Default");
                    break;
                }
            }
        }
    });
    
    return default_registry;
}

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
    (void)time_seconds;

    for (auto& command : draw_list.commands) {
        if (command.kind != DrawCommandKind::TexturedQuad || !command.textured_quad.has_value()) {
            continue;
        }

        auto& quad = *command.textured_quad;
        if (quad.texture.surface != nullptr || quad.texture.id.empty()) {
            continue;
        }

        const std::string& id = quad.texture.id;
        
        if (id.compare(0, 6, "image:") == 0) {
            std::string layer_id = id.substr(6);
            bool found = false;
            for (const auto& comp : scene.compositions) {
                for (const auto& layer : comp.layers) {
                    if (layer.id == layer_id) {
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
