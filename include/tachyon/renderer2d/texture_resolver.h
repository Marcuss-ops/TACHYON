#pragma once

#include "tachyon/renderer2d/draw_list_builder.h"
#include "tachyon/media/media_manager.h"
#include "tachyon/core/spec/scene_spec.h"
#include "tachyon/text/subtitle.h"

#include <memory>

namespace tachyon { class BitmapFont; }

namespace tachyon::renderer2d {

class TextRenderConfig {
public:
    static TextRenderConfig& instance();
    void set_font(const BitmapFont* font) { font_ = font; }
    const BitmapFont* font() const { return font_; }
    void set_subtitle_entries(const std::vector<text::SubtitleEntry>* entries) { subtitle_entries_ = entries; }
    const std::vector<text::SubtitleEntry>* subtitle_entries() const { return subtitle_entries_; }

private:
    TextRenderConfig() = default;
    const BitmapFont* font_ = nullptr;
    const std::vector<text::SubtitleEntry>* subtitle_entries_ = nullptr;
};

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
