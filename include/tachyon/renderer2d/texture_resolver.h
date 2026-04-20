#pragma once

#include "tachyon/renderer2d/draw_list_builder.h"
#include "tachyon/media/media_manager.h"
#include "tachyon/core/spec/scene_spec.h"

namespace tachyon::renderer2d {

/**
 * Resolves TextureHandle string IDs to actual SurfaceRGBA pointers.
 */
class TextureResolver {
public:
    static void resolve_textures(
        DrawList2D& draw_list, 
        const SceneSpec& scene,
        media::MediaManager& media_manager);

private:
    static media::AlphaMode parse_alpha_mode(const std::optional<std::string>& mode);
};

} // namespace tachyon::renderer2d
