#include "tachyon/text/layout/layout.h"
#include "layout_engine_helpers.h"

namespace tachyon::text {

TextHitTestResult TextLayoutResult::hit_test(std::int32_t x, std::int32_t y) const {
    TextHitTestResult result;
    if (glyphs.empty()) return result;

    const TextLine* best_line = nullptr;
    std::int32_t min_y_dist = 1000000;
    
    for (const auto& line : lines) {
        std::int32_t dist = 0;
        if (y < line.y) dist = line.y - y;
        else if (y > line.y + line_height) dist = y - (line.y + line_height);
        
        if (dist < min_y_dist) {
            min_y_dist = dist;
            best_line = &line;
        }
    }

    if (!best_line) return result;

    std::size_t closest_glyph_idx = best_line->glyph_start_index;
    std::int32_t min_x_dist = 1000000;

    for (std::size_t i = 0; i < best_line->glyph_count; ++i) {
        const auto& g = glyphs[best_line->glyph_start_index + i];
        std::int32_t centerX = g.x + g.width / 2;
        std::int32_t dist = std::abs(x - centerX);
        
        if (dist < min_x_dist) {
            min_x_dist = dist;
            closest_glyph_idx = best_line->glyph_start_index + i;
        }
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
            target_glyph = &g;
            break;
        }
    }

    if (target_glyph) {
        return {target_glyph->x, target_glyph->y, 2, target_glyph->height};
    }

    const auto& last = glyphs.back();
    return {last.x + last.width, last.y, 2, last.height};
}

math::RectF ResolvedTextLayout::get_range_bounds(std::size_t start_char_index, std::size_t end_char_index) const {
    if (start_char_index >= end_char_index || glyphs.empty()) {
        return {};
    }

    math::RectF bounds{};
    bool have_bounds = false;
    for (const auto& glyph : glyphs) {
        if (glyph.source_index < start_char_index || glyph.source_index >= end_char_index) {
            continue;
        }
        bounds = have_bounds ? union_rects(bounds, glyph.bounds) : glyph.bounds;
        have_bounds = true;
    }

    return have_bounds ? bounds : math::RectF{};
}

} // namespace tachyon::text
