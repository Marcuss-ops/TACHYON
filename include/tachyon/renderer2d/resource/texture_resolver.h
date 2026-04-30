#pragma once

#include "tachyon/renderer2d/raster/draw_list_builder.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/text/content/subtitle.h"
#include "tachyon/text/fonts/core/font.h"
#include "tachyon/text/fonts/core/font_registry.h"

#include <memory>
#include <vector>



namespace tachyon::renderer2d {

/**
 * Utility to find and load a default system font for text rendering.
 * Returns nullptr if no suitable font is found.
 */
const ::tachyon::text::FontRegistry* get_default_font_registry();

/**
 * Resolves TextureHandle string IDs to actual SurfaceRGBA pointers.
 */
class TextureResolver {
public:
    static void resolve_textures(
        DrawList2D& draw_list, 
        const SceneSpec& scene,
        media::MediaManager& media_manager,
        double time_seconds = 0.0);

private:
    static media::AlphaMode parse_alpha_mode(const std::optional<std::string>& mode);
};

} // namespace tachyon::renderer2d
