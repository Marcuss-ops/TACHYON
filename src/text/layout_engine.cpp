#include "tachyon/text/layout.h"
#include "tachyon/renderer2d/text/utf8/utf8_decoder.h"
#include "tachyon/renderer2d/text/shaping/font_shaping.h"
#include <algorithm>
#include <cmath>
#include <string_view>
#include <vector>

namespace tachyon::text {

namespace {

std::uint32_t choose_scale(const BitmapFont& font, const TextStyle& style) {
    const std::uint32_t requested = style.pixel_size == 0 ? static_cast<std::uint32_t>(font.line_height()) : style.pixel_size;
    const std::uint32_t base = static_cast<std::uint32_t>(std::max(1, font.line_height()));
    return std::max<std::uint32_t>(1, requested / base);
}

bool is_breakable_space(std::uint32_t codepoint) {
    return codepoint == static_cast<std::uint32_t>(' ') || codepoint == static_cast<std::uint32_t>('\t');
}

std::int32_t aligned_x_offset(TextAlignment alignment, std::uint32_t box_width, std::int32_t line_width) {
    if (box_width == 0U) return 0;
    const std::int32_t available = static_cast<std::int32_t>(box_width) - line_width;
    if (available <= 0) return 0;
    switch (alignment) {
        case TextAlignment::Center: return available / 2;
        case TextAlignment::Right:  return available;
        case TextAlignment::Left:
        default:                    return 0;
    }
}

void finalize_line(TextLayoutResult& result, std::size_t start_index, std::size_t glyph_count, std::int32_t line_width, std::int32_t line_y, TextAlignment alignment, std::uint32_t box_width) {
    if (glyph_count == 0U) return;
    const std::int32_t offset_x = aligned_x_offset(alignment, box_width, line_width);
    for (std::size_t index = start_index; index < start_index + glyph_count; ++index) {
        result.glyphs[index].x += offset_x;
    }
    result.lines.push_back(TextLine{ start_index, glyph_count, line_width, line_y });
    const std::uint32_t used_width = box_width == 0U ? static_cast<std::uint32_t>(std::max(0, line_width)) : std::min(box_width, static_cast<std::uint32_t>(std::max(0, line_width + offset_x)));
    result.width = std::max(result.width, used_width);
}

} // namespace

TextLayoutResult layout_text(const BitmapFont& font, std::string_view utf8_text, const TextStyle& style, const TextBox& text_box, TextAlignment alignment, const TextLayoutOptions& options) {
    TextLayoutResult result;
    if (!font.is_loaded()) return result;

    const std::uint32_t scale = choose_scale(font, style);
    const std::int32_t scaled_line_height = font.line_height() * static_cast<std::int32_t>(scale);
    const std::int32_t wrap_width = static_cast<std::int32_t>(text_box.width);
    result.scale = scale;
    result.line_height = scaled_line_height;

    const auto codepoints = renderer2d::text::utf8::decode_utf8(utf8_text);

    std::int32_t pen_x = 0, pen_y = 0, current_line_width = 0;
    std::size_t line_start = 0, last_break_codepoint_index = 0, last_break_glyph_index = 0, current_word_index = 0;
    std::int32_t last_break_line_width = 0;
    bool have_break = false, last_was_space = true;
    const std::int32_t tracking_advance = static_cast<std::int32_t>(std::lround(options.tracking * static_cast<float>(scale)));

    if (font.has_freetype_face()) {
        for (std::size_t index = 0; index < codepoints.size();) {
            const std::uint32_t codepoint = codepoints[index];
            if (codepoint == (std::uint32_t)'\n') {
                finalize_line(result, line_start, result.glyphs.size() - line_start, current_line_width, pen_y, alignment, text_box.width);
                pen_x = 0; current_line_width = 0; pen_y += scaled_line_height; line_start = result.glyphs.size(); have_break = false; last_was_space = true; ++index; continue;
            }
            if (is_breakable_space(codepoint)) {
                const GlyphBitmap* glyph = font.find_glyph(codepoint);
                if (glyph) {
                    if (!last_was_space && !result.glyphs.empty()) { have_break = true; last_break_codepoint_index = index; last_break_glyph_index = result.glyphs.size(); last_break_line_width = current_line_width; ++current_word_index; }
                    const std::int32_t ga = std::max(1, glyph->advance_x) * (std::int32_t)scale;
                    result.glyphs.push_back(PositionedGlyph{ codepoint, font.glyph_index_for_codepoint(codepoint), pen_x + glyph->x_offset * (std::int32_t)scale, pen_y + (font.ascent() - (std::int32_t)glyph->height - glyph->y_offset) * (std::int32_t)scale, (std::int32_t)glyph->width * (std::int32_t)scale, (std::int32_t)glyph->height * (std::int32_t)scale, ga, result.glyphs.size(), current_word_index, true });
                    pen_x += ga + tracking_advance; current_line_width = pen_x; last_was_space = true;
                }
                ++index; continue;
            }
            const std::size_t run_start = index;
            while (index < codepoints.size() && codepoints[index] != (std::uint32_t)'\n' && !is_breakable_space(codepoints[index])) ++index;
            const renderer2d::text::shaping::ShapedGlyphRun shaped = renderer2d::text::shaping::shape_run_with_harfbuzz(font, std::vector<std::uint32_t>(codepoints.begin() + run_start, codepoints.begin() + index), scale);
            if (!last_was_space && !result.glyphs.empty()) ++current_word_index;
            if (wrap_width > 0 && text_box.multiline && options.word_wrap && pen_x > 0 && (pen_x + shaped.width) > wrap_width) {
                if (have_break && last_break_glyph_index > line_start) { result.glyphs.resize(last_break_glyph_index); finalize_line(result, line_start, last_break_glyph_index - line_start, last_break_line_width, pen_y, alignment, text_box.width); index = last_break_codepoint_index; }
                else finalize_line(result, line_start, result.glyphs.size() - line_start, current_line_width, pen_y, alignment, text_box.width);
                pen_x = 0; current_line_width = 0; pen_y += scaled_line_height; line_start = result.glyphs.size(); have_break = false; last_was_space = true; continue;
            }
            for (const auto& gr : shaped.glyphs) {
                if (const auto* g = font.find_glyph_by_index(gr.font_glyph_index)) {
                    result.glyphs.push_back(PositionedGlyph{ gr.codepoint, gr.font_glyph_index, pen_x + g->x_offset * (std::int32_t)scale + gr.offset_x, pen_y + (font.ascent() - (std::int32_t)g->height - g->y_offset) * (std::int32_t)scale + gr.offset_y, (std::int32_t)g->width * (std::int32_t)scale, (std::int32_t)g->height * (std::int32_t)scale, gr.advance_x, result.glyphs.size(), current_word_index, false });
                    pen_x += gr.advance_x + tracking_advance; current_line_width = pen_x;
                }
            }
            last_was_space = false;
        }
    } else {
        for (std::size_t index = 0; index < codepoints.size(); ++index) {
            const std::uint32_t codepoint = codepoints[index];
            if (codepoint == (std::uint32_t)'\n') {
                finalize_line(result, line_start, result.glyphs.size() - line_start, current_line_width, pen_y, alignment, text_box.width);
                pen_x = 0; current_line_width = 0; pen_y += scaled_line_height; line_start = result.glyphs.size(); have_break = false; last_was_space = true; continue;
            }
            if (const auto* glyph = font.find_glyph(codepoint)) {
                const bool ws = is_breakable_space(codepoint);
                if (!ws && last_was_space && !result.glyphs.empty()) ++current_word_index;
                const std::int32_t ga = std::max(1, glyph->advance_x) * (std::int32_t)scale;
                std::int32_t kerning = (index > 0 && !last_was_space) ? font.get_kerning(codepoints[index - 1], codepoint) * (std::int32_t)scale : 0;
                if (text_box.multiline && options.word_wrap && wrap_width > 0 && pen_x > 0 && (pen_x + ga + kerning) > wrap_width && !ws) {
                    if (have_break && last_break_glyph_index > line_start) { result.glyphs.resize(last_break_glyph_index); finalize_line(result, line_start, last_break_glyph_index - line_start, last_break_line_width, pen_y, alignment, text_box.width); index = last_break_codepoint_index; }
                    else finalize_line(result, line_start, result.glyphs.size() - line_start, current_line_width, pen_y, alignment, text_box.width);
                    pen_x = 0; current_line_width = 0; pen_y += scaled_line_height; line_start = result.glyphs.size(); have_break = false; last_was_space = true; current_word_index = 0; continue;
                }
                pen_x += kerning;
                result.glyphs.push_back(PositionedGlyph{ codepoint, font.glyph_index_for_codepoint(codepoint), pen_x + glyph->x_offset * (std::int32_t)scale, pen_y + (font.ascent() - static_cast<std::int32_t>(glyph->height) - glyph->y_offset) * (std::int32_t)scale, static_cast<std::int32_t>(glyph->width) * (std::int32_t)scale, static_cast<std::int32_t>(glyph->height) * (std::int32_t)scale, ga, result.glyphs.size(), current_word_index, ws });
                pen_x += ga + tracking_advance; current_line_width = pen_x;
                if (ws) { have_break = true; last_break_codepoint_index = index; last_break_glyph_index = result.glyphs.size(); last_break_line_width = current_line_width; }
                last_was_space = ws;
            }
        }
    }

    finalize_line(result, line_start, result.glyphs.size() - line_start, current_line_width, pen_y, alignment, text_box.width);
    if (!result.lines.empty()) result.height = (std::uint32_t)(result.lines.back().y + scaled_line_height);
    else result.height = (std::uint32_t)scaled_line_height;
    if (text_box.height > 0U) result.height = std::min(result.height, text_box.height);
    if (result.width == 0U) result.width = text_box.width > 0U ? text_box.width : 0U;
    return result;
}

} // namespace tachyon::text
