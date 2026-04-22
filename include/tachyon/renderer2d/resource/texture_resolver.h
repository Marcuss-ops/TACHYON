#pragma once

#include "tachyon/renderer2d/raster/draw_list_builder.h"
#include "tachyon/media/media_manager.h"
#include "tachyon/core/spec/scene_spec.h"
#include "tachyon/text/subtitle.h"
#include "tachyon/text/font.h"

#include <memory>
#include <vector>



namespace tachyon::renderer2d {

/**
 * Utility to find and load a default system font for text rendering.
 * Returns nullptr if no suitable font is found.
 */
const ::tachyon::text::Font* get_default_text_font();

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
