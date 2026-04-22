#include "tachyon/text/layout/layout.h"
#include "tachyon/text/i18n/bidi_engine.h"
#include "tachyon/text/fonts/font_registry.h"
#include "tachyon/text/i18n/script_detector.h"
#include "tachyon/renderer2d/text/utf8/utf8_decoder.h"
#include "tachyon/renderer2d/text/shaping/font_shaping.h"
#include "tachyon/renderer2d/text/shaping/shaping_cache.h"
#include "tachyon/text/layout/cluster_iterator.h"
#include "tachyon/text/layout/line_breaker.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <vector>

namespace tachyon::text {

using namespace tachyon::renderer2d::text::shaping;

namespace {

std::uint32_t choose_scale(const BitmapFont& font, const TextStyle& style) {
    const std::uint32_t requested = style.pixel_size == 0 ? static_cast<std::uint32_t>(font.line_height()) : style.pixel_size;
    const std::uint32_t base = static_cast<std::uint32_t>(std::max(1, font.line_height()));
    return std::max<std::uint32_t>(1, requested / base);
}

bool is_breakable_space(std::uint32_t codepoint) {
    return codepoint == static_cast<std::uint32_t>(' ') || 
           codepoint == static_cast<std::uint32_t>('\t') ||
           codepoint == 0x00A0; // non-breaking space
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
        default:                    return 0;
    }
}

void finalize_line(TextLayoutResult& result, std::size_t start_index, std::size_t glyph_count, std::int32_t line_width, std::int32_t line_y, TextAlignment alignment, std::uint32_t box_width, bool last_line) {
    if (glyph_count == 0U) return;
    
    std::int32_t offset_x = aligned_x_offset(alignment, box_width, line_width);
    
    // Justification
    if (alignment == TextAlignment::Justify && !last_line && box_width > 0U) {
        const std::int32_t available = static_cast<std::int32_t>(box_width) - line_width;
        if (available > 0) {
            std::size_t spaces_count = 0;
            for (std::size_t index = start_index; index < start_index + glyph_count; ++index) {
                if (result.glyphs[index].whitespace) spaces_count++;
            }
            if (spaces_count > 0) {
                const std::int32_t extra_per_space = available / static_cast<std::int32_t>(spaces_count);
                std::int32_t current_extra = 0;
                for (std::size_t index = start_index; index < start_index + glyph_count; ++index) {
                    result.glyphs[index].x += current_extra;
                    if (result.glyphs[index].whitespace) {
                        current_extra += extra_per_space;
                    }
                }
                line_width = static_cast<std::int32_t>(box_width);
            }
        }
    } else {
        for (std::size_t index = start_index; index < start_index + glyph_count; ++index) {
            result.glyphs[index].x += offset_x;
        }
    }

    result.lines.push_back(TextLine{ start_index, glyph_count, line_width, line_y });
    const std::uint32_t used_width = box_width == 0U ? static_cast<std::uint32_t>(std::max(0, line_width)) : std::min(box_width, static_cast<std::uint32_t>(std::max(0, line_width + offset_x)));
    result.width = std::max(result.width, used_width);
}

struct SubRun {
    const Font* font;
    std::vector<std::uint32_t> codepoints;
    CharacterDirection direction;
    std::size_t start_index; // index in original codepoint sequence
};

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
        if (!ws && last_was_space && !result.glyphs.empty()) {
            ++current_word_index;
        }

        const auto* g = sub.font->find_glyph_by_index(gr.font_glyph_index);
        if (!g) {
            continue;
        }

        const std::int32_t glyph_advance = std::abs(gr.advance_x) + tracking_advance;
        if (rtl) {
            cursor -= glyph_advance;
        }

        std::size_t global_cp_index = sub.start_index + gr.cluster;
        std::size_t cluster_idx = 0;
        std::size_t cp_count = 1;
        std::size_t cp_start = global_cp_index;
        
        for (std::size_t c = 0; c < clusters.size(); ++c) {
            if (global_cp_index >= clusters[c].start_index && global_cp_index < clusters[c].start_index + clusters[c].length) {
                cluster_idx = c;
                cp_start = clusters[c].start_index;
                cp_count = clusters[c].length;
                break;
            }
        }

        result.glyphs.push_back(PositionedGlyph{
            gr.codepoint,
            gr.font_glyph_index,
            sub.font->id(),
            cursor + g->x_offset * static_cast<std::int32_t>(scale) + gr.offset_x,
            pen_y + (sub.font->ascent() - static_cast<std::int32_t>(g->height) - g->y_offset) * static_cast<std::int32_t>(scale) + gr.offset_y,
            static_cast<std::int32_t>(g->width) * static_cast<std::int32_t>(scale),
            static_cast<std::int32_t>(g->height) * static_cast<std::int32_t>(scale),
            gr.advance_x,
            result.glyphs.size(),
            current_word_index,
            cluster_idx,
            cp_start,
            cp_count,
            ws
        });

        if (!rtl) {
            cursor += glyph_advance;
        }
        consumed += glyph_advance;
        last_was_space = ws;
    }

    return rtl ? pen_x + std::max<std::int32_t>(consumed, std::abs(shaped.width)) : std::max(pen_x, cursor);
}

} // namespace

TextHitTestResult TextLayoutResult::hit_test(std::int32_t x, std::int32_t y) const {
    TextHitTestResult result;
    if (glyphs.empty()) return result;

    // 1. Find the line closest to Y.
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

    // 2. Find the glyph in that line closest to X.
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
    
    // Determine whether the hit lands on the leading or trailing edge of the cluster.
    // We approximate this by comparing the distance to the cluster bounds.
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
        // If it's the trailing edge, the logical insertion point is the next cluster.
        result.codepoint_index = g.cluster_codepoint_start + g.cluster_codepoint_count;
        result.cluster_index = g.cluster_index + 1;
    }

    return result;
}

renderer2d::RectI TextLayoutResult::get_caret_rect(std::size_t codepoint_index) const {
    if (glyphs.empty()) return {0, 0, 2, line_height};

    // Find the glyph that starts or contains this codepoint
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

    // If not found (e.g. at the very end), use the end of the last glyph
    const auto& last = glyphs.back();
    return {last.x + last.width, last.y, 2, last.height};
}

class InternalLayoutEngine {
public:
    static TextLayoutResult layout(
        const std::vector<const Font*>& fallback_chain,
        std::string_view utf8_text,
        const TextStyle& style,
        const TextBox& text_box,
        TextAlignment alignment,
        const TextLayoutOptions& options) {

        TextLayoutResult result;
        if (fallback_chain.empty()) return result;

        const auto codepoints = renderer2d::text::utf8::decode_utf8(utf8_text);
        if (codepoints.empty()) return result;

        const auto bidi_runs = BidiEngine::analyze(codepoints);
        const Font& primary_font = *fallback_chain.front();
        const std::uint32_t scale = choose_scale(primary_font, style);
        const std::int32_t scaled_line_height = primary_font.line_height() * static_cast<std::int32_t>(scale);
        const std::int32_t wrap_width = static_cast<std::int32_t>(text_box.width);
        
        result.scale = scale;
        result.line_height = scaled_line_height;

        std::int32_t pen_x = 0, pen_y = 0, current_line_width = 0;
        std::size_t line_start = 0, current_word_index = 0;
        bool last_was_space = true;
        const std::int32_t tracking_advance = static_cast<std::int32_t>(std::lround(options.tracking * static_cast<float>(scale)));

        auto& cache = ShapingCache::get_instance();

        const auto clusters = ClusterIterator::segment(codepoints);

        for (const auto& run : bidi_runs) {
            std::vector<SubRun> subruns;
            
            for (std::size_t i = 0; i < run.length; ) {
                std::size_t cp_index = run.start + i;
                
                // Find which cluster this codepoint belongs to
                const GraphemeCluster* current_cluster = nullptr;
                for (const auto& c : clusters) {
                    if (cp_index >= c.start_index && cp_index < c.start_index + c.length) {
                        current_cluster = &c;
                        break;
                    }
                }
                
                if (!current_cluster) { i++; continue; }

                // Find best font for the whole cluster
                const Font* selected_font = nullptr;
                for (const auto& f : fallback_chain) {
                    bool all_supported = true;
                    for (std::size_t k = 0; k < current_cluster->length; ++k) {
                        std::uint32_t cp = codepoints[current_cluster->start_index + k];
                        if (!(f->has_glyph(cp) || cp == '\n' || cp == ' ' || cp == '\t' || cp == 0x200D)) {
                            all_supported = false;
                            break;
                        }
                    }
                    if (all_supported) {
                        selected_font = f;
                        break;
                    }
                }
                if (!selected_font) selected_font = fallback_chain.front();

                // Group clusters with same font into subruns
                if (subruns.empty() || subruns.back().font != selected_font) {
                    subruns.push_back({selected_font, {}, run.direction, current_cluster->start_index});
                }
                
                // Add all codepoints of the cluster to the subrun
                // (Note: we might need to clip the cluster if it overlaps BiDi run boundaries, 
                // but usually they don't. For simplicity, we'll just add the whole cluster).
                for (std::size_t k = 0; k < current_cluster->length; ++k) {
                    subruns.back().codepoints.push_back(codepoints[current_cluster->start_index + k]);
                }
                
                // Skip to the end of the cluster (relative to run start)
                const std::size_t cluster_end_index = current_cluster->start_index + current_cluster->length;
                if (cluster_end_index > run.start) {
                    i = cluster_end_index - run.start;
                } else {
                    i++;
                }
            }

            for (const auto& sub : subruns) {
                const int dir = (sub.direction == CharacterDirection::RTL) ? 1 : 0;
                ScriptInfo script_info = ScriptDetector::detect_run_info(sub.codepoints.data(), sub.codepoints.size());

                ShapingCacheKey cache_key;
                cache_key.font_id = sub.font->id();
                cache_key.scale = scale;
                cache_key.direction = dir;
                cache_key.script = script_info.script;
                cache_key.language = script_info.language;
                cache_key.codepoints = sub.codepoints;
                cache_key.features = style.features;

                ShapedGlyphRun shaped;
                if (!cache.get(cache_key, shaped)) {
                    shaped = shape_run_with_harfbuzz(*sub.font, sub.codepoints, scale, script_info.script, script_info.language, dir, style.features);
                    cache.put(cache_key, shaped);
                }

                // Check for line break opportunities in this subrun
                // (Simplified: we'll check if the subrun itself contains or is a break opportunity)
                bool contains_break = false;
                for (std::uint32_t cp : sub.codepoints) {
                    if (is_breakable_space(cp) || cp == '-' || (cp >= 0x4E00 && cp <= 0x9FFF)) {
                        contains_break = true;
                        break;
                    }
                }

                if (wrap_width > 0 && text_box.multiline && options.word_wrap && pen_x > 0 && std::abs(shaped.width) > 0 && (pen_x + std::abs(shaped.width)) > wrap_width) {
                    // Only wrap if we had a previous break opportunity or if this subrun contains one
                    // For now, we still do simple wrapping but we could improve this to backtrack.
                    finalize_line(result, line_start, result.glyphs.size() - line_start, current_line_width, pen_y, alignment, text_box.width, false);
                    pen_x = 0;
                    current_line_width = 0;
                    pen_y += scaled_line_height;
                    line_start = result.glyphs.size();
                    last_was_space = true;
                }

                pen_x = place_shaped_run(result, pen_x, pen_y, sub, shaped, scale, tracking_advance, current_word_index, last_was_space, clusters);
                current_line_width = std::max(current_line_width, pen_x);
            }
        }

        finalize_line(result, line_start, result.glyphs.size() - line_start, current_line_width, pen_y, alignment, text_box.width, true);
        if (!result.lines.empty()) result.height = (std::uint32_t)(result.lines.back().y + scaled_line_height);
        else result.height = (std::uint32_t)scaled_line_height;
        if (text_box.height > 0U) result.height = std::min(result.height, text_box.height);
        if (result.width == 0U) result.width = text_box.width > 0U ? text_box.width : 0U;
        return result;
    }
};

TextLayoutResult layout_text(const BitmapFont& font, std::string_view utf8_text, const TextStyle& style, const TextBox& text_box, TextAlignment alignment, const TextLayoutOptions& options) {
    if (!font.is_loaded()) return {};
    std::vector<const Font*> chain = { &font };
    return InternalLayoutEngine::layout(chain, utf8_text, style, text_box, alignment, options);
}

TextLayoutResult layout_text(
    const FontRegistry& registry,
    const std::string& font_name,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    const TextLayoutOptions& options) {

    const auto fallback_chain = registry.get_fallback_chain(font_name);
    if (fallback_chain.empty()) {
        return {};
    }
    return InternalLayoutEngine::layout(fallback_chain, utf8_text, style, text_box, alignment, options);
}

TextRasterSurface rasterize_layout_debug(const TextLayoutResult& layout) {
    TextRasterSurface surface(layout.width, layout.height);
    if (layout.width == 0 || layout.height == 0) return surface;

    // Draw lines
    for (const auto& line : layout.lines) {
        // Draw the baseline anchor used by the layout result.
        surface.draw_line(0, line.y, layout.width, line.y, renderer2d::Color{0.0f, 1.0f, 0.0f, 0.5f}); // Green for baseline
        surface.draw_line(0, line.y + layout.line_height, layout.width, line.y + layout.line_height, renderer2d::Color{1.0f, 0.0f, 0.0f, 0.2f}); // Red for line bottom
    }

    // Draw glyph boxes and clusters
    for (const auto& g : layout.glyphs) {
        // Glyph bounding box
        surface.draw_rect(g.x, g.y, g.width, g.height, renderer2d::Color{1.0f, 1.0f, 1.0f, 0.3f}); // White for glyph
        
        // Cluster indicator (small bar at bottom)
        renderer2d::Color cluster_color = (g.cluster_index % 2 == 0) ? renderer2d::Color{1.0f, 1.0f, 0.0f, 0.8f} : renderer2d::Color{0.0f, 1.0f, 1.0f, 0.8f};
        surface.draw_line(g.x, g.y + g.height - 2, g.x + g.width, g.y + g.height - 2, cluster_color);
    }

    return surface;
}

} // namespace tachyon::text
