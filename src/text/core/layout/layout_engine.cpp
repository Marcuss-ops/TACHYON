#include <iostream>
#include "layout_engine_helpers.h"
#include "tachyon/text/core/low_level/utf8/utf8_decoder.h"
#include "tachyon/text/layout/line_breaker.h"
#include "tachyon/text/rendering/text_raster_surface.h"

namespace tachyon::text {

using namespace tachyon::renderer2d::text::shaping;

namespace {
} // namespace

void place_shaped_glyph(
    TextLayoutResult& result,
    std::int32_t& cursor,
    std::int32_t pen_y,
    const SubRun& sub,
    const ShapedGlyphRun::Glyph& gr,
    std::uint32_t scale,
    std::int32_t tracking_advance,
    std::size_t& current_word_index,
    bool& last_was_space,
    const std::vector<GraphemeCluster>& clusters) {

    const bool rtl = sub.direction == CharacterDirection::RTL;
    const bool ws = is_breakable_space(gr.codepoint);
    
    if (!ws && last_was_space && !result.glyphs.empty()) {
        ++current_word_index;
    }

    const auto* g = sub.font->find_glyph_by_index(gr.font_glyph_index);
    if (!g) {
        // Skip missing glyphs
        return;
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
        ws,
        rtl,
        sub.font
    });

    if (!rtl) {
        cursor += glyph_advance;
    }
    last_was_space = ws;
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
        std::cerr << "DEBUG: codepoints=" << codepoints.size() << ", bidi_runs=" << bidi_runs.size() << ", clusters=" << clusters.size() << "\n";

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
                    subruns.push_back(SubRun{selected_font, {}, run.direction, current_cluster->start_index});
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
            
            std::cerr << "DEBUG: subruns count=" << subruns.size() << "\n";

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
                    if (sub.font->has_freetype_face()) {
                        shaped = shape_run_with_harfbuzz(*sub.font, sub.codepoints, scale, script_info.script, script_info.language, dir, style.features);
                    } else {
                        shaped = shape_run_simple(*sub.font, sub.codepoints, scale);
                    }
                    cache.put(cache_key, shaped);
                }
                
                std::cerr << "DEBUG: shaped glyphs count=" << shaped.glyphs.size() << " (width=" << shaped.width << ")\n";

                // Group glyphs into words for wrapping
                struct Word {
                    std::vector<std::size_t> glyph_indices;
                    std::int32_t width = 0;
                    bool ends_with_break = false;
                };

                std::vector<Word> words;
                Word current_word;
                for (std::size_t i = 0; i < shaped.glyphs.size(); ++i) {
                    const auto& gr = shaped.glyphs[i];
                    current_word.glyph_indices.push_back(i);
                    current_word.width += std::abs(gr.advance_x) + tracking_advance;
                    if (is_breakable_space(gr.codepoint) || gr.codepoint == '-') {
                        current_word.ends_with_break = true;
                        words.push_back(std::move(current_word));
                        current_word = {};
                    }
                }
                if (!current_word.glyph_indices.empty()) words.push_back(std::move(current_word));

                const bool rtl = sub.direction == CharacterDirection::RTL;

                for (const auto& word : words) {
                    bool wrap_needed = (wrap_width > 0 && text_box.multiline && options.word_wrap && pen_x > 0 && (pen_x + word.width) > (std::int32_t)wrap_width);
                    
                    if (wrap_needed) {
                        finalize_line(result, line_start, result.glyphs.size() - line_start, current_line_width, pen_y, alignment, text_box.width, false);
                        pen_x = 0;
                        current_line_width = 0;
                        pen_y += scaled_line_height;
                        line_start = result.glyphs.size();
                        last_was_space = true;
                    }

                    bool is_start_of_line = (pen_x == 0);
                    for (std::size_t idx : word.glyph_indices) {
                        const auto& gr = shaped.glyphs[idx];
                        if (is_start_of_line && is_breakable_space(gr.codepoint)) continue;
                        
                        std::int32_t glyph_width = std::abs(gr.advance_x) + tracking_advance;
                        std::int32_t cursor = rtl ? pen_x + glyph_width : pen_x;
                        place_shaped_glyph(result, cursor, pen_y, sub, gr, scale, tracking_advance, current_word_index, last_was_space, clusters);
                        pen_x += glyph_width;
                        is_start_of_line = false;
                    }
                    current_line_width = std::max(current_line_width, pen_x);
                }
                std::cerr << "DEBUG: result.glyphs.size()=" << result.glyphs.size() << "\n";
            }
        }

        finalize_line(result, line_start, result.glyphs.size() - line_start, current_line_width, pen_y, alignment, text_box.width, true);
        if (!result.lines.empty()) result.height = (std::uint32_t)(result.lines.back().y + scaled_line_height);
        else result.height = (std::uint32_t)scaled_line_height;
        if (text_box.height > 0U) result.height = std::min(result.height, text_box.height);
        if (result.width == 0U) result.width = text_box.width > 0U ? text_box.width : 0U;
        sync_resolved_layout(result, primary_font, style);
        std::cerr << "DEBUG: InternalLayoutEngine::layout returning result with glyphs=" << result.glyphs.size() << ", lines=" << result.lines.size() << "\n";
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

    auto fallback_chain = registry.get_fallback_chain(font_name);
    if (fallback_chain.empty()) {
        // Font not found - try default font
        const Font* default_font = registry.default_font();
        if (default_font) {
            fallback_chain = {default_font};
        } else {
            return {};
        }
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
