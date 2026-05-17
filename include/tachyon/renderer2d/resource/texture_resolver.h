#pragma once

#include "tachyon/renderer2d/raster/draw_list_builder.h"
#include "tachyon/core/media/media_provider.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#ifdef TACHYON_ENABLE_TEXT
#include "tachyon/text/fonts/core/font_registry.h"
#endif

#include <memory>
#include <vector>



namespace tachyon::renderer2d {

/**
 * Utility to find and load a default system font for text rendering.
 * Returns nullptr if no suitable font is found.
 */
#ifdef TACHYON_ENABLE_TEXT
const ::tachyon::text::FontRegistry* get_default_font_registry();
#endif

/**
 * Resolves TextureHandle string IDs to actual SurfaceRGBA pointers.
 */
class TextureResolver {
public:
    static void resolve_textures(
        DrawList2D& draw_list, 
        const SceneSpec& scene,
        media::IMediaProvider& media_manager,
        double time_seconds = 0.0);

private:
    static ::tachyon::AlphaMode parse_alpha_mode(const std::optional<std::string>& mode);
};

} // namespace tachyon::renderer2d
