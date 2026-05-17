#include "tachyon/renderer2d/resource/texture_resolver.h"

#include <iostream>
#include <memory>
#include <vector>

#ifdef TACHYON_ENABLE_TEXT
#include "tachyon/text/core/low_level/freetype/freetype_manager.h"
#endif

namespace tachyon::renderer2d {

#ifdef TACHYON_ENABLE_TEXT
const ::tachyon::text::FontRegistry* get_default_font_registry() {
#ifdef TACHYON_ENABLE_TEXT
    // Mandate: Initialize FreeType BEFORE FontRegistry so it is destroyed AFTER it.
    ::tachyon::renderer2d::text::get_ft_library();
#endif
    
    static ::tachyon::text::FontRegistry default_registry;
    static std::once_flag once;
    
    std::call_once(once, []() {
        struct FontCandidate {
            std::string name;
            std::filesystem::path path;
        };

        const std::vector<FontCandidate> candidates = {
            // High-quality bundled fonts
            {"SFPro", "assets/fonts/SFPRODISPLAYBOLD.OTF"},
            {"SFPro", "../assets/fonts/SFPRODISPLAYBOLD.OTF"},
            {"SFPro", "../../assets/fonts/SFPRODISPLAYBOLD.OTF"},
            {"SegoeUI", "assets/fonts/SegoeUI.ttf"},
            {"SegoeUI", "../assets/fonts/SegoeUI.ttf"},
            
            // Modern Windows system fonts
            {"Bahnschrift", R"(C:\Windows\Fonts\bahnschrift.ttf)"},
            {"Calibri", R"(C:\Windows\Fonts\calibri.ttf)"},
            {"Consolas", R"(C:\Windows\Fonts\consola.ttf)"},
            {"Cambria", R"(C:\Windows\Fonts\cambria.ttc)"},
            {"Impact", R"(C:\Windows\Fonts\impact.ttf)"},
            {"Courier New", R"(C:\Windows\Fonts\cour.ttf)"},
            
            // Windows system fonts
            {"Arial", R"(C:\Windows\Fonts\arial.ttf)"},
            {"Arial", R"(C:\Windows\Fonts\Arial.ttf)"},
            {"Arial", R"(C:\Windows\Fonts\arialuni.ttf)"},
            {"LiberationSans", R"(C:\Windows\Fonts\LiberationSans-Regular.ttf)"},
            {"DejaVuSans", R"(C:\Windows\Fonts\DejaVuSans.ttf)"},
            
            // Linux system fonts
            {"LiberationSans", "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"},
            {"DejaVuSans", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"},
            {"FreeSans", "/usr/share/fonts/truetype/freefont/FreeSans.ttf"},
            {"Ubuntu", "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf"}
        };
        
        bool default_set = false;
        for (const auto& candidate : candidates) {
            if (std::filesystem::exists(candidate.path)) {
                // Register with its specific name if not already registered
                if (!default_registry.find(candidate.name)) {
                    default_registry.load_ttf(candidate.name, candidate.path, 48U);
                }

                // Use the first successful high-quality font as "Default"
                if (!default_set) {
                    if (default_registry.load_ttf("Default", candidate.path, 48U)) {
                        default_registry.set_default("Default");
                        default_set = true;
                    }
                }
            }
        }
    });
    
    return &default_registry;
}
#endif

::tachyon::AlphaMode TextureResolver::parse_alpha_mode(const std::optional<std::string>& mode) {
    if (!mode.has_value()) return ::tachyon::AlphaMode::Straight;
    if (*mode == "premultiplied") return ::tachyon::AlphaMode::Premultiplied;
    if (*mode == "ignore") return ::tachyon::AlphaMode::Ignore;
    return ::tachyon::AlphaMode::Straight;
}

void TextureResolver::resolve_textures(
    DrawList2D& draw_list, 
    const SceneSpec& scene,
    media::IMediaProvider& media_manager,
    double time_seconds) {
    (void)time_seconds;

    for (auto& command : draw_list.commands) {
        if (command.kind != DrawCommandKind::TexturedRect || !command.textured_rect.has_value()) {
            continue;
        }

        auto& tex_rect = *command.textured_rect;
        if (tex_rect.texture.surface != nullptr || tex_rect.texture.id.empty()) {
            continue;
        }

        const std::string& id = tex_rect.texture.id;
        
        if (id.compare(0, 6, "image:") == 0) {
            std::string layer_id = id.substr(6);
            bool found = false;
            for (const auto& comp : scene.compositions) {
                for (const auto& layer : comp.layers) {
                    if (layer.identity.id == layer_id) {
                        for (const auto& asset : scene.assets) {
                            if (asset.id == layer.identity.id || asset.id == layer.identity.name) {
                                const ::tachyon::AlphaMode mode = parse_alpha_mode(asset.alpha_mode);
                                tex_rect.texture.surface = media_manager.get_image(asset.path, mode);
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
