#include <hb-ft.h>
#include <hb.h>
#include <cmath>
#include <algorithm>

#include "tachyon/renderer2d/text/shaping/font_shaping.h"
#include "tachyon/text/fonts/core/font.h"
#include "tachyon/text/layout/layout.h"

namespace tachyon::renderer2d::text::shaping {

ShapedGlyphRun shape_run_with_harfbuzz(
    const BitmapFont& font, 
    const std::vector<std::uint32_t>& codepoints, 
    std::uint32_t scale,
    const char* script,
    const char* language,
    int direction,
    const std::vector<tachyon::text::TextFeature>& features) {
    
    ShapedGlyphRun run;
    if (!font.has_freetype_face() || codepoints.empty()) {
        return run;
    }

    hb_font_t* hb_font = hb_ft_font_create_referenced(static_cast<FT_Face>(font.freetype_face()));
    if (hb_font == nullptr) {
        return run;
    }

    hb_buffer_t* buffer = hb_buffer_create();
    hb_buffer_add_utf32(buffer, codepoints.data(), static_cast<int>(codepoints.size()), 0, static_cast<int>(codepoints.size()));
    
    if (script) hb_buffer_set_script(buffer, hb_script_from_string(script, -1));
    if (language) hb_buffer_set_language(buffer, hb_language_from_string(language, -1));
    hb_buffer_set_direction(buffer, direction == 1 ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
    
    hb_buffer_guess_segment_properties(buffer);
    hb_buffer_set_cluster_level(buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
    
    std::vector<hb_feature_t> hb_features;
    hb_features.reserve(features.size());
    for (const auto& feat : features) {
        hb_feature_t hbf;
        if (hb_feature_from_string(feat.tag.c_str(), -1, &hbf)) {
            hbf.value = feat.value;
            hb_features.push_back(hbf);
        }
    }

    hb_shape(hb_font, buffer, hb_features.data(), static_cast<unsigned int>(hb_features.size()));

    unsigned int glyph_count = 0;
    hb_glyph_info_t* infos = hb_buffer_get_glyph_infos(buffer, &glyph_count);
    hb_glyph_position_t* positions = hb_buffer_get_glyph_positions(buffer, &glyph_count);

    run.glyphs.reserve(glyph_count);
    for (unsigned int i = 0; i < glyph_count; ++i) {
        const std::uint32_t glyph_index = infos[i].codepoint;
        const std::int32_t advance_x = static_cast<std::int32_t>(std::round(static_cast<float>(positions[i].x_advance) / 64.0f));
        const std::int32_t offset_x = static_cast<std::int32_t>(std::round(static_cast<float>(positions[i].x_offset) / 64.0f));
        const std::int32_t offset_y = static_cast<std::int32_t>(std::round(static_cast<float>(-positions[i].y_offset) / 64.0f));
        const std::uint32_t source_index = static_cast<std::uint32_t>(std::min<std::size_t>(infos[i].cluster, codepoints.size() - 1U));
        run.glyphs.push_back(ShapedGlyphRun::Glyph{
            codepoints[source_index],
            glyph_index,
            advance_x * static_cast<std::int32_t>(scale),
            offset_x * static_cast<std::int32_t>(scale),
            offset_y * static_cast<std::int32_t>(scale),
            infos[i].cluster
        });
        run.width += advance_x * static_cast<std::int32_t>(scale);
    }

    hb_buffer_destroy(buffer);
    hb_font_destroy(hb_font);
    return run;
}

} // namespace tachyon::renderer2d::text::shaping