#include "layout_engine_helpers.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

namespace tachyon::text {

namespace {
} // namespace

::tachyon::ColorSpec to_color_spec(const renderer2d::Color& color) {
    auto to_channel = [](float value) -> std::uint8_t {
        const float clamped = std::clamp(value, 0.0f, 1.0f);
        return static_cast<std::uint8_t>(std::lround(clamped * 255.0f));
    };
    return {to_channel(color.r), to_channel(color.g), to_channel(color.b), to_channel(color.a)};
}

math::RectF make_rect(float x, float y, float width, float height) {
    return {x, y, width, height};
}

math::RectF union_rects(const math::RectF& a, const math::RectF& b) {
    if (a.width <= 0.0f || a.height <= 0.0f) return b;
    if (b.width <= 0.0f || b.height <= 0.0f) return a;
    const float left = std::min(a.x, b.x);
    const float top = std::min(a.y, b.y);
    const float right = std::max(a.x + a.width, b.x + b.width);
    const float bottom = std::max(a.y + a.height, b.y + b.height);
    return {left, top, right - left, bottom - top};
}

bool rect_is_empty(const math::RectF& rect) {
    return rect.width <= 0.0f || rect.height <= 0.0f;
}

std::uint32_t choose_scale(const BitmapFont& font, const TextStyle& style) {
    const std::uint32_t requested = style.pixel_size == 0 ? static_cast<std::uint32_t>(font.line_height()) : style.pixel_size;
    const std::uint32_t base = static_cast<std::uint32_t>(std::max(1, font.line_height()));
    return std::max<std::uint32_t>(1, requested / base);
}

float resolved_glyph_advance(
    const Font& font,
    const ::tachyon::renderer2d::text::shaping::ShapedGlyphRun::Glyph& gr,
    std::uint32_t scale,
    bool fixed_pitch) {

    if (!fixed_pitch) {
        return static_cast<float>(gr.advance_x);
    }

    const float cell_advance = std::max(
        1.0f,
        static_cast<float>(font.default_advance()) * static_cast<float>(scale));
    return cell_advance;
}

bool is_breakable_space(std::uint32_t codepoint) {
    return codepoint == static_cast<std::uint32_t>(' ') || 
           codepoint == static_cast<std::uint32_t>('\t') || codepoint == 0x00A0;
}

float aligned_x_offset(HorizontalAlign alignment, float box_width, float line_width) {
    if (box_width <= 0.0f) return 0.0f;
    const float available = box_width - line_width;
    if (available <= 0.0f) return 0.0f;
    switch (alignment) {
        case HorizontalAlign::Center: return available * 0.5f;
        case HorizontalAlign::Right:  return available;
        case HorizontalAlign::Left:
        case HorizontalAlign::Justify:
        default: return 0.0f;
    }
}

void finalize_line(TextLayoutResult& result, std::size_t start_index, std::size_t glyph_count, float line_width, float line_y, HorizontalAlign alignment, float box_width, bool last_line, float tracking_advance) {
    if (glyph_count == 0U) return;
    
    // Calculate width excluding trailing whitespace and its tracking
    std::size_t actual_count = glyph_count;
    while (actual_count > 0 && result.glyphs[start_index + actual_count - 1].whitespace) {
        actual_count--;
    }

    float effective_width = 0.0f;
    for (size_t i = 0; i < actual_count; ++i) {
        effective_width += result.glyphs[start_index + i].advance_x;
        if (i + 1 < actual_count) {
            effective_width += tracking_advance;
        }
    }


    if (alignment == HorizontalAlign::Justify && !last_line && box_width > 0.0f) {
        const float available = box_width - effective_width;
        if (available > 0.0f) {
            std::size_t spaces_count = 0;
            for (std::size_t index = start_index; index < start_index + actual_count; ++index) {
                if (result.glyphs[index].whitespace) spaces_count++;
            }
            if (spaces_count > 0) {
                const float space_add = available / static_cast<float>(spaces_count);
                float current_add = 0.0f;
                for (std::size_t i = start_index; i < result.glyphs.size(); ++i) {
                    if (result.glyphs[i].whitespace) {
                        current_add += space_add;
                    }
                    result.glyphs[i].position.x += current_add;
                }
                effective_width = box_width;
            }
        }
    }

    TextLine line;
    line.glyph_start_index = start_index;
    line.glyph_count = actual_count;
    line.width = effective_width;
    line.y = line_y;
    
    // Calculate ink bounds for the line
    math::RectF ink_rect{};
    bool first_glyph = true;
    for (size_t i = 0; i < actual_count; ++i) {
        const auto& g = result.glyphs[start_index + i];
        if (g.whitespace) continue;
        if (first_glyph) {
            ink_rect = g.bounds;
            first_glyph = false;
        } else {
            ink_rect = union_rects(ink_rect, g.bounds);
        }
    }
    line.ink_bounds = ink_rect;
    line.logical_bounds = {0.0f, line_y, effective_width, static_cast<float>(result.line_height)};

    float offset_x = aligned_x_offset(alignment, box_width, effective_width);

    // Optical horizontal centering
    if (alignment == HorizontalAlign::Center && !rect_is_empty(line.ink_bounds)) {
        float ink_center_x = line.ink_bounds.x + line.ink_bounds.width * 0.5f;
        float desired_center_x = box_width * 0.5f;
        float shift_x = desired_center_x - ink_center_x;

        for (std::size_t i = start_index; i < result.glyphs.size(); ++i) {
            result.glyphs[i].position.x += shift_x;
            result.glyphs[i].bounds.x += shift_x;
        }
        line.ink_bounds.x += shift_x;
        line.logical_bounds.x += shift_x;
    } else if (std::abs(offset_x) > 0.0001f) {
        for (std::size_t i = start_index; i < result.glyphs.size(); ++i) {
            result.glyphs[i].position.x += offset_x;
            result.glyphs[i].bounds.x += offset_x;
        }
        line.ink_bounds.x += offset_x;
        line.logical_bounds.x += offset_x;
    }

    result.lines.push_back(line);
    result.content_width = std::max(result.content_width, effective_width);
}

float compute_vertical_offset(float box_height, float content_height, VerticalAlign align) {
    switch (align) {
        case VerticalAlign::Top:
            return 0.0f;
        case VerticalAlign::Middle:
            return (box_height - content_height) * 0.5f;
        case VerticalAlign::Bottom:
            return box_height - content_height;
    }
    return 0.0f;
}

float place_shaped_run(
    TextLayoutResult& result,
    float pen_x,
    float pen_y,
    const SubRun& sub,
    const ShapedGlyphRun& shaped,
    std::uint32_t scale,
    bool fixed_pitch,
    float tracking_advance,
    std::size_t& current_word_index,
    bool& last_was_space,
    const std::vector<GraphemeCluster>& clusters) {
    const bool rtl = sub.direction == CharacterDirection::RTL;
    const float s = static_cast<float>(scale);
    float logical_run_width = 0.0f;
    for (const auto& gr : shaped.glyphs) {
        logical_run_width += resolved_glyph_advance(*sub.font, gr, scale, fixed_pitch) + tracking_advance;
    }
    float cursor = rtl ? pen_x + logical_run_width : pen_x;
    float consumed = 0.0f;
    for (const auto& gr : shaped.glyphs) {
        const bool ws = is_breakable_space(gr.codepoint);
        if (!ws && last_was_space && !result.glyphs.empty()) ++current_word_index;
        const auto* g = sub.font->find_glyph_by_index(gr.font_glyph_index);
        if (!g) continue;
        const float logical_advance = resolved_glyph_advance(*sub.font, gr, scale, fixed_pitch);
        const float glyph_advance = logical_advance + tracking_advance;
        if (rtl) cursor -= glyph_advance;
        std::size_t global_cp_index = sub.start_index + gr.cluster;
        std::size_t cluster_idx = 0, cp_count = 1, cp_start = global_cp_index;
        for (std::size_t c = 0; c < clusters.size(); ++c) {
            if (global_cp_index >= clusters[c].start_index && global_cp_index < clusters[c].start_index + clusters[c].length) {
                cluster_idx = c; cp_start = clusters[c].start_index; cp_count = clusters[c].length; break;
            }
        }
        
        PositionedGlyph pg;
        pg.codepoint = gr.codepoint;
        pg.font_glyph_index = gr.font_glyph_index;
        pg.font_id = sub.font->font_id();
        pg.position.x = cursor + static_cast<float>(g->x_offset) * s + static_cast<float>(gr.offset_x);
        pg.position.y = pen_y + static_cast<float>(sub.font->ascent() - static_cast<std::int32_t>(g->height) - g->y_offset) * s + static_cast<float>(gr.offset_y);
        pg.width = static_cast<float>(g->width) * s;
        pg.height = static_cast<float>(g->height) * s;
        pg.advance_x = logical_advance;
        pg.glyph_index = result.glyphs.size();
        pg.word_index = current_word_index;
        pg.cluster_index = cluster_idx;
        pg.cluster_codepoint_start = cp_start;
        pg.cluster_codepoint_count = cp_count;
        pg.whitespace = ws;
        pg.is_rtl = rtl;
        pg.resolved_font = sub.font;
        pg.bounds = {pg.position.x, pg.position.y, pg.width, pg.height};
        
        result.glyphs.push_back(pg);
        if (!rtl) cursor += glyph_advance;
        consumed += glyph_advance;
        last_was_space = ws;
    }
    return rtl ? pen_x + std::max(consumed, logical_run_width) : std::max(pen_x, cursor);
}

TextHitTestResult TextLayoutResult::hit_test(std::int32_t x, std::int32_t y) const {
    TextHitTestResult result;
    if (glyphs.empty()) return result;
    const TextLine* best_line = nullptr;
    float min_y_dist = 1000000.0f;
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);
    for (const auto& line : lines) {
        float dist = 0.0f;
        if (fy < line.y) dist = line.y - fy;
        else if (fy > line.y + static_cast<float>(line_height)) dist = fy - (line.y + static_cast<float>(line_height));
        if (dist < min_y_dist) { min_y_dist = dist; best_line = &line; }
    }
    if (!best_line) return result;
    std::size_t closest_glyph_idx = best_line->glyph_start_index;
    float min_x_dist = 1000000.0f;
    for (std::size_t i = 0; i < best_line->glyph_count; ++i) {
        const auto& g = glyphs[best_line->glyph_start_index + i];
        float centerX = g.position.x + g.width * 0.5f;
        float dist = std::abs(fx - centerX);
        if (dist < min_x_dist) { min_x_dist = dist; closest_glyph_idx = best_line->glyph_start_index + i; }
    }
    const auto& g = glyphs[closest_glyph_idx];
    result.inside_text = true;
    result.cluster_index = g.cluster_index;
    result.codepoint_index = g.cluster_codepoint_start;
    float cluster_min_x = g.position.x;
    float cluster_max_x = g.position.x + g.width;
    for (const auto& cg : glyphs) {
        if (cg.cluster_index == g.cluster_index) {
            cluster_min_x = std::min(cluster_min_x, cg.position.x);
            cluster_max_x = std::max(cluster_max_x, cg.position.x + cg.width);
        }
    }
    if (std::abs(fx - cluster_min_x) < std::abs(fx - cluster_max_x)) {
        result.is_leading_edge = true;
    } else {
        result.is_leading_edge = false;
        result.codepoint_index = g.cluster_codepoint_start + g.cluster_codepoint_count;
        result.cluster_index = g.cluster_index + 1;
    }
    return result;
}

renderer2d::RectI TextLayoutResult::get_caret_rect(std::size_t codepoint_index) const {
    if (glyphs.empty()) return {0, 0, 2, line_height};
    const PositionedGlyph* target_glyph = nullptr;
    for (const auto& g : glyphs) {
        if (codepoint_index >= g.cluster_codepoint_start && codepoint_index < g.cluster_codepoint_start + g.cluster_codepoint_count) {
            target_glyph = &g; break;
        }
    }
    if (target_glyph) return {static_cast<int>(std::lround(target_glyph->position.x)), static_cast<int>(std::lround(target_glyph->position.y)), 2, static_cast<int>(std::lround(target_glyph->height))};
    const auto& last = glyphs.back();
    return {static_cast<int>(std::lround(last.position.x + last.width)), static_cast<int>(std::lround(last.position.y)), 2, static_cast<int>(std::lround(last.height))};
}

math::RectF TextLayoutResult::get_range_bounds(std::size_t start_char_index, std::size_t end_char_index) const {
    if (start_char_index >= end_char_index || glyphs.empty()) return {};
    math::RectF res_bounds{}; bool have_bounds = false;
    for (const auto& glyph : glyphs) {
        if (glyph.cluster_codepoint_start < start_char_index || glyph.cluster_codepoint_start >= end_char_index) continue;
        res_bounds = have_bounds ? union_rects(res_bounds, glyph.bounds) : glyph.bounds;
        have_bounds = true;
    }
    return have_bounds ? res_bounds : math::RectF{};
}

} // namespace tachyon::text
