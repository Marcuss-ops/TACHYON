#ifndef TACHYON_TEXT_CORE_LAYOUT_LAYOUT_ENGINE_HELPERS_H
#define TACHYON_TEXT_CORE_LAYOUT_LAYOUT_ENGINE_HELPERS_H

#include "tachyon/text/layout/layout.h"
#include "tachyon/renderer2d/text/shaping/font_shaping.h"
#include "tachyon/text/layout/layout_cache.h"
#include "tachyon/renderer2d/text/shaping/shaping_cache.h"
#include "tachyon/text/layout/cluster_iterator.h"
#include "tachyon/text/i18n/bidi_engine.h"
#include "tachyon/text/i18n/script_detector.h"
#include "tachyon/text/fonts/font_registry.h"
#include <vector>
#include <cstdint>
#include <map>
#include <string_view>

namespace tachyon::text {

using namespace tachyon::renderer2d::text::shaping;

struct SubRun {
    const Font* font = nullptr;
    std::vector<std::uint32_t> codepoints;
    CharacterDirection direction = CharacterDirection::LTR;
    std::size_t start_index = 0;
};

::tachyon::ColorSpec to_color_spec(const renderer2d::Color& color);
math::RectF make_rect(float x, float y, float width, float height);
math::RectF union_rects(const math::RectF& a, const math::RectF& b);
bool rect_is_empty(const math::RectF& rect);
std::uint32_t choose_scale(const BitmapFont& font, const TextStyle& style);
bool is_breakable_space(std::uint32_t codepoint);
std::int32_t aligned_x_offset(TextAlignment alignment, std::uint32_t box_width, std::int32_t line_width);
void finalize_line(TextLayoutResult& result, std::size_t start_index, std::size_t glyph_count, std::int32_t line_width, std::int32_t line_y, TextAlignment alignment, std::uint32_t box_width, bool last_line);
std::int32_t place_shaped_run(
    TextLayoutResult& result,
    std::int32_t pen_x,
    std::int32_t pen_y,
    const SubRun& sub,
    const ShapedGlyphRun& shaped,
    std::uint32_t scale,
    std::int32_t tracking_advance,
    std::size_t& current_word_index,
    bool& last_was_space,
    const std::vector<GraphemeCluster>& clusters);
void sync_resolved_layout(TextLayoutResult& result, const BitmapFont& font, const TextStyle& style);

} // namespace tachyon::text

#endif
