#include "layout_engine_helpers.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <vector>

namespace tachyon::text {

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

bool is_breakable_space(std::uint32_t codepoint) {
    return codepoint == static_cast<std::uint32_t>(' ') || 
           codepoint == static_cast<std::uint32_t>('\t') || codepoint == 0x00A0;
}

std::int32_t aligned_x_offset(TextAlignment alignment, std::uint32_t box_width, std::int32_t line_width) {
    if (box_width == 0U) return 0;
    const std::int32_t available = static_cast<std::int32_t>(box_width) - line_width;
    if (available <= 0) return 0;
    switch (alignment) {
        case TextAlignment::Center: return available / 2;
        case TextAlignment::Right:  return available;
        case TextAlignment::Left:
        case TextAlignment::Justify:
        default: return 0;
    }
}

void finalize_line(TextLayoutResult& result, std::size_t start_index, std::size_t glyph_count, std::int32_t line_width, std::int32_t line_y, TextAlignment alignment, std::uint32_t box_width, bool last_line) {
    if (glyph_count == 0U) return;
    // Calculate width excluding trailing whitespace
    std::int32_t effective_width = line_width;
    std::size_t actual_count = glyph_count;
    while (actual_count > 0 && result.glyphs[start_index + actual_count - 1].whitespace) {
        effective_width -= result.glyphs[start_index + actual_count - 1].advance_x;
        actual_count--;
    }
    std::cerr << "DEBUG: finalize_line start=" << start_index << " count=" << glyph_count << " actual=" << actual_count << " line_width=" << line_width << " effective=" << effective_width << "\n";

    std::int32_t offset_x = aligned_x_offset(alignment, box_width, effective_width);
    if (alignment == TextAlignment::Justify && !last_line && box_width > 0U) {
        const std::int32_t available = static_cast<std::int32_t>(box_width) - effective_width;
        if (available > 0) {
            std::size_t spaces_count = 0;
            for (std::size_t index = start_index; index < start_index + actual_count; ++index) {
                if (result.glyphs[index].whitespace) spaces_count++;
            }
            if (spaces_count > 0) {
                const std::int32_t extra_per_space = available / static_cast<std::int32_t>(spaces_count);
                std::int32_t current_extra = 0;
                for (std::size_t index = start_index; index < start_index + glyph_count; ++index) {
                    result.glyphs[index].x += current_extra;
                    if (index < start_index + actual_count && result.glyphs[index].whitespace) current_extra += extra_per_space;
                }
                line_width = static_cast<std::int32_t>(box_width);
            }
        }
    } else {
        for (std::size_t index = start_index; index < start_index + glyph_count; ++index) {
            result.glyphs[index].x += offset_x;
        }
    }
    result.lines.push_back(TextLine{ start_index, actual_count, effective_width, line_y });
    const std::uint32_t used_width = box_width == 0U ? static_cast<std::uint32_t>(std::max(0, effective_width)) : std::min(box_width, static_cast<std::uint32_t>(std::max(0, effective_width + offset_x)));
    result.width = std::max(result.width, used_width);
}

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
    const std::vector<GraphemeCluster>& clusters) {
    const bool rtl = sub.direction == CharacterDirection::RTL;
    std::int32_t cursor = rtl ? pen_x + shaped.width : pen_x;
    std::int32_t consumed = 0;
    for (const auto& gr : shaped.glyphs) {
        const bool ws = is_breakable_space(gr.codepoint);
        if (!ws && last_was_space && !result.glyphs.empty()) ++current_word_index;
        const auto* g = sub.font->find_glyph_by_index(gr.font_glyph_index);
        if (!g) continue;
        const std::int32_t glyph_advance = std::abs(gr.advance_x) + tracking_advance;
        if (rtl) cursor -= glyph_advance;
        std::size_t global_cp_index = sub.start_index + gr.cluster;
        std::size_t cluster_idx = 0, cp_count = 1, cp_start = global_cp_index;
        for (std::size_t c = 0; c < clusters.size(); ++c) {
            if (global_cp_index >= clusters[c].start_index && global_cp_index < clusters[c].start_index + clusters[c].length) {
                cluster_idx = c; cp_start = clusters[c].start_index; cp_count = clusters[c].length; break;
            }
        }
        result.glyphs.push_back(PositionedGlyph{
            gr.codepoint, gr.font_glyph_index, sub.font->id(),
            cursor + g->x_offset * static_cast<std::int32_t>(scale) + gr.offset_x,
            pen_y + (sub.font->ascent() - static_cast<std::int32_t>(g->height) - g->y_offset) * static_cast<std::int32_t>(scale) + gr.offset_y,
            static_cast<std::int32_t>(g->width) * static_cast<std::int32_t>(scale),
            static_cast<std::int32_t>(g->height) * static_cast<std::int32_t>(scale),
            gr.advance_x, result.glyphs.size(), current_word_index,
            cluster_idx, cp_start, cp_count, ws, rtl, sub.font
        });
        if (!rtl) cursor += glyph_advance;
        consumed += glyph_advance;
        last_was_space = ws;
    }
    return rtl ? pen_x + std::max<std::int32_t>(consumed, std::abs(shaped.width)) : std::max(pen_x, cursor);
}

/**
 * @brief Synchronizes the external-facing ResolvedTextLayout (float) from the internal TextLayoutResult (integer).
 *
 * This function acts as the bridge between the internal layout engine (using PositionedGlyph)
 * and the external systems (animators, path modifiers) that use ResolvedGlyph.
 * It copies all relevant data and converts integer coordinates to float.
 */
void sync_resolved_layout(TextLayoutResult& result, const BitmapFont& font, const TextStyle& style) {
    result.ResolvedTextLayout::glyphs.clear();
    result.ResolvedTextLayout::runs.clear();
    result.ResolvedTextLayout::lines.clear();
    result.ResolvedTextLayout::clusters.clear();
    result.ResolvedTextLayout::total_bounds = {};
    result.ResolvedTextLayout::is_on_path = false;
    const float font_size = style.pixel_size == 0U ? static_cast<float>(std::max(1, font.line_height())) : static_cast<float>(style.pixel_size);
    std::map<std::size_t, std::vector<std::size_t>> cluster_to_glyphs;
    for (std::size_t i = 0; i < result.glyphs.size(); ++i) {
        const auto& g = result.glyphs[i];
        cluster_to_glyphs[g.cluster_index].push_back(i);
    }
    for (const auto& [cluster_idx, glyph_indices] : cluster_to_glyphs) {
        if (glyph_indices.empty()) continue;
        const auto& first_glyph = result.glyphs[glyph_indices.front()];
        GlyphCluster cluster;
        cluster.source_text_start = first_glyph.cluster_codepoint_start;
        cluster.source_text_length = first_glyph.cluster_codepoint_count;
        cluster.glyph_start = glyph_indices.front();
        cluster.glyph_count = glyph_indices.size();
        result.ResolvedTextLayout::clusters.push_back(cluster);
    }
    result.ResolvedTextLayout::glyphs.reserve(result.glyphs.size());
    for (const auto& g : result.glyphs) {
        ResolvedGlyph resolved;
        resolved.codepoint = g.codepoint;
        resolved.font_glyph_index = g.font_glyph_index;
        resolved.font_id = g.font_id;
        resolved.position = {static_cast<float>(g.x), static_cast<float>(g.y)};
        resolved.advance_x = static_cast<float>(g.advance_x);
        resolved.advance_y = 0.0f;
        resolved.font = g.resolved_font;
        resolved.source_index = g.cluster_codepoint_start;
        resolved.word_index = g.word_index;
        resolved.cluster_index = g.cluster_index;
        resolved.is_space = g.whitespace;
        resolved.whitespace = g.whitespace;
        resolved.is_rtl = g.is_rtl;
        resolved.font_size = font_size;
        resolved.fill_color = to_color_spec(style.fill_color);
        resolved.stroke_color = {0, 0, 0, 0};
        resolved.stroke_width = 0.0f;
        resolved.opacity = 1.0f;
        resolved.scale = {1.0f, 1.0f};
        resolved.rotation = 0.0f;
        resolved.blur_radius = 0.0f;
        resolved.reveal_factor = 1.0f;
        resolved.bounds = make_rect(static_cast<float>(g.x), static_cast<float>(g.y), static_cast<float>(g.width), static_cast<float>(g.height));
        result.ResolvedTextLayout::glyphs.push_back(resolved);
        result.ResolvedTextLayout::total_bounds = rect_is_empty(result.ResolvedTextLayout::total_bounds) ? resolved.bounds : union_rects(result.ResolvedTextLayout::total_bounds, resolved.bounds);
    }
    if (!result.ResolvedTextLayout::glyphs.empty()) {
        std::size_t run_start = 0;
        const Font* run_font = result.ResolvedTextLayout::glyphs.front().font;
        math::RectF run_bounds = result.ResolvedTextLayout::glyphs.front().bounds;
        for (std::size_t i = 1; i <= result.ResolvedTextLayout::glyphs.size(); ++i) {
            const bool run_break = i == result.ResolvedTextLayout::glyphs.size() || result.ResolvedTextLayout::glyphs[i].font != run_font;
            if (i < result.ResolvedTextLayout::glyphs.size()) run_bounds = union_rects(run_bounds, result.ResolvedTextLayout::glyphs[i].bounds);
            if (run_break) {
                result.ResolvedTextLayout::runs.push_back(ResolvedTextRun{run_start, i - run_start, run_font, font_size, run_bounds});
                if (i < result.ResolvedTextLayout::glyphs.size()) {
                    run_start = i; run_font = result.ResolvedTextLayout::glyphs[i].font; run_bounds = result.ResolvedTextLayout::glyphs[i].bounds;
                }
            }
        }
    }
    result.ResolvedTextLayout::lines.reserve(result.lines.size());
    for (const auto& line : result.lines) {
        math::RectF line_bounds{}; bool have_bounds = false;
        const std::size_t line_end = std::min(result.glyphs.size(), line.glyph_start_index + line.glyph_count);
        for (std::size_t i = line.glyph_start_index; i < line_end; ++i) {
            const auto& g = result.glyphs[i];
            const math::RectF glyph_bounds = make_rect(static_cast<float>(g.x), static_cast<float>(g.y), static_cast<float>(g.width), static_cast<float>(g.height));
            line_bounds = have_bounds ? union_rects(line_bounds, glyph_bounds) : glyph_bounds;
            have_bounds = true;
        }
        result.ResolvedTextLayout::lines.push_back(ResolvedTextLine{
            line.glyph_start_index, line.glyph_count, static_cast<float>(line.y),
            static_cast<float>(font.ascent()), static_cast<float>(font.descent()),
            have_bounds ? line_bounds : make_rect(0.0f, static_cast<float>(line.y), static_cast<float>(line.width), static_cast<float>(result.line_height))
        });
    }
    for (std::size_t line_index = 0; line_index < result.ResolvedTextLayout::lines.size(); ++line_index) {
        const auto& line = result.ResolvedTextLayout::lines[line_index];
        const std::size_t end = std::min(result.ResolvedTextLayout::glyphs.size(), line.start_glyph_index + line.length);
        for (std::size_t glyph_index = line.start_glyph_index; glyph_index < end; ++glyph_index) {
            result.ResolvedTextLayout::glyphs[glyph_index].line_index = line_index;
        }
    }
}

TextHitTestResult TextLayoutResult::hit_test(std::int32_t x, std::int32_t y) const {
    TextHitTestResult result;
    if (glyphs.empty()) return result;
    const TextLine* best_line = nullptr;
    std::int32_t min_y_dist = 1000000;
    for (const auto& line : lines) {
        std::int32_t dist = 0;
        if (y < line.y) dist = line.y - y;
        else if (y > line.y + line_height) dist = y - (line.y + line_height);
        if (dist < min_y_dist) { min_y_dist = dist; best_line = &line; }
    }
    if (!best_line) return result;
    std::size_t closest_glyph_idx = best_line->glyph_start_index;
    std::int32_t min_x_dist = 1000000;
    for (std::size_t i = 0; i < best_line->glyph_count; ++i) {
        const auto& g = glyphs[best_line->glyph_start_index + i];
        std::int32_t centerX = g.x + g.width / 2;
        std::int32_t dist = std::abs(x - centerX);
        if (dist < min_x_dist) { min_x_dist = dist; closest_glyph_idx = best_line->glyph_start_index + i; }
    }
    const auto& g = glyphs[closest_glyph_idx];
    result.inside_text = true;
    result.cluster_index = g.cluster_index;
    result.codepoint_index = g.cluster_codepoint_start;
    std::int32_t cluster_min_x = g.x;
    std::int32_t cluster_max_x = g.x + g.width;
    for (const auto& cg : glyphs) {
        if (cg.cluster_index == g.cluster_index) {
            cluster_min_x = std::min(cluster_min_x, cg.x);
            cluster_max_x = std::max(cluster_max_x, cg.x + cg.width);
        }
    }
    if (std::abs(x - cluster_min_x) < std::abs(x - cluster_max_x)) {
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
    if (target_glyph) return {target_glyph->x, target_glyph->y, 2, target_glyph->height};
    const auto& last = glyphs.back();
    return {last.x + last.width, last.y, 2, last.height};
}

math::RectF ResolvedTextLayout::get_range_bounds(std::size_t start_char_index, std::size_t end_char_index) const {
    if (start_char_index >= end_char_index || glyphs.empty()) return {};
    math::RectF bounds{}; bool have_bounds = false;
    for (const auto& glyph : glyphs) {
        if (glyph.source_index < start_char_index || glyph.source_index >= end_char_index) continue;
        bounds = have_bounds ? union_rects(bounds, glyph.bounds) : glyph.bounds;
        have_bounds = true;
    }
    return have_bounds ? bounds : math::RectF{};
}

} // namespace tachyon::text
