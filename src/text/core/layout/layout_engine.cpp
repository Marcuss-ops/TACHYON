#include <iostream>
#include "layout_engine_helpers.h"
#include "tachyon/text/core/low_level/utf8/utf8_decoder.h"
#include "tachyon/text/layout/line_breaker.h"
#include "tachyon/text/rendering/text_raster_surface.h"
#include "tachyon/text/layout/layout_cache.h"

namespace tachyon::text {

using namespace tachyon::renderer2d::text::shaping;

namespace {
} // namespace

void place_shaped_glyph(
    TextLayoutResult& result,
    float& cursor,
    float pen_y,
    const SubRun& sub,
    const ShapedGlyphRun::Glyph& gr,
    std::uint32_t scale,
    float tracking_advance,
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
        return;
    }

    const float s = static_cast<float>(scale);
    const float glyph_advance = static_cast<float>(std::abs(gr.advance_x)) + tracking_advance;
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

    PositionedGlyph pg;
    pg.codepoint = gr.codepoint;
    pg.font_glyph_index = gr.font_glyph_index;
    pg.font_id = sub.font->font_id();
    pg.position.x = cursor + static_cast<float>(g->x_offset) * s + static_cast<float>(gr.offset_x);
    pg.position.y = pen_y + static_cast<float>(sub.font->ascent() - static_cast<std::int32_t>(g->height) - g->y_offset) * s + static_cast<float>(gr.offset_y);
    pg.width = static_cast<float>(g->width) * s;
    pg.height = static_cast<float>(g->height) * s;
    pg.advance_x = static_cast<float>(gr.advance_x);
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
        const TextBoxSpec& text_box,
        const TextLayoutOptions& options) {

        TextLayoutResult result;
        if (fallback_chain.empty()) return result;

        const auto codepoints = renderer2d::text::utf8::decode_utf8(utf8_text);
        if (codepoints.empty()) return result;

        const auto bidi_runs = BidiEngine::analyze(codepoints);
        const Font& primary_font = *fallback_chain.front();
        const std::uint32_t scale = choose_scale(primary_font, style);
        const float scaled_line_height = static_cast<float>(primary_font.line_height()) * static_cast<float>(scale);
        const float wrap_width = text_box.width;
        
        result.scale = scale;
        result.line_height = static_cast<std::int32_t>(std::lround(scaled_line_height));

        float pen_x = 0.0f, pen_y = 0.0f, current_line_width = 0.0f;
        std::size_t line_start = 0, current_word_index = 0;
        bool last_was_space = true;
        const float tracking_advance = options.tracking * static_cast<float>(scale);

        auto& cache = ShapingCache::get_instance();
        const auto clusters = ClusterIterator::segment(codepoints);

        for (const auto& run : bidi_runs) {
            std::vector<SubRun> subruns;
            
            for (std::size_t i = 0; i < run.length; ) {
                std::size_t cp_index = run.start + i;
                const GraphemeCluster* current_cluster = nullptr;
                for (const auto& c : clusters) {
                    if (cp_index >= c.start_index && cp_index < c.start_index + c.length) {
                        current_cluster = &c;
                        break;
                    }
                }
                
                if (!current_cluster) { i++; continue; }

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

                if (subruns.empty() || subruns.back().font != selected_font) {
                    subruns.push_back(SubRun{selected_font, {}, run.direction, current_cluster->start_index});
                }
                
                for (std::size_t k = 0; k < current_cluster->length; ++k) {
                    subruns.back().codepoints.push_back(codepoints[current_cluster->start_index + k]);
                }
                
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
                cache_key.font_id = sub.font->font_id();
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
                
                struct Word {
                    std::vector<std::size_t> glyph_indices;
                    float width = 0.0f;
                    bool ends_with_break = false;
                };

                std::vector<Word> words;
                Word current_word;
                for (std::size_t i = 0; i < shaped.glyphs.size(); ++i) {
                    const auto& gr = shaped.glyphs[i];
                    current_word.glyph_indices.push_back(i);
                    current_word.width += static_cast<float>(std::abs(gr.advance_x)) + tracking_advance;
                    if (is_breakable_space(gr.codepoint) || gr.codepoint == '-') {
                        current_word.ends_with_break = true;
                        words.push_back(std::move(current_word));
                        current_word = {};
                    }
                }
                if (!current_word.glyph_indices.empty()) words.push_back(std::move(current_word));

                const bool rtl = sub.direction == CharacterDirection::RTL;

                for (const auto& word : words) {
                    bool wrap_needed = (wrap_width > 0.0f && text_box.mode == TextBoxMode::Fixed && options.word_wrap && pen_x > 0.0f && (pen_x + word.width) > wrap_width);
                    
                    if (wrap_needed) {
                        finalize_line(result, line_start, result.glyphs.size() - line_start, current_line_width, pen_y, text_box.horizontal_align, static_cast<std::uint32_t>(std::max(0.0f, text_box.width)), false);
                        pen_x = 0.0f;
                        current_line_width = 0.0f;
                        pen_y += scaled_line_height;
                        line_start = result.glyphs.size();
                        last_was_space = true;
                    }

                    bool is_start_of_line = (pen_x == 0.0f);
                    for (std::size_t idx : word.glyph_indices) {
                        const auto& gr = shaped.glyphs[idx];
                        if (is_start_of_line && is_breakable_space(gr.codepoint)) continue;
                        
                        float glyph_width = static_cast<float>(std::abs(gr.advance_x)) + tracking_advance;
                        float cursor = rtl ? pen_x + glyph_width : pen_x;
                        place_shaped_glyph(result, cursor, pen_y, sub, gr, scale, tracking_advance, current_word_index, last_was_space, clusters);
                        pen_x += glyph_width;
                        is_start_of_line = false;
                    }
                    current_line_width = std::max(current_line_width, pen_x);
                }
            }
        }

        finalize_line(result, line_start, result.glyphs.size() - line_start, current_line_width, pen_y, text_box.horizontal_align, static_cast<std::uint32_t>(std::max(0.0f, text_box.width)), true);
        
        if (!result.lines.empty()) {
            result.content_height = result.lines.back().y + scaled_line_height;
        } else {
            result.content_height = scaled_line_height;
        }

        if (text_box.mode == TextBoxMode::Auto) {
            result.box_width = result.content_width;
            result.box_height = result.content_height;
        } else {
            result.box_width = text_box.width;
            result.box_height = text_box.height;
        }

        result.offset_y = compute_vertical_offset(result.box_height, result.content_height, text_box.vertical_align);
        if (result.offset_y != 0.0f) {
            for (auto& g : result.glyphs) {
                g.position.y += result.offset_y;
                g.bounds.y += result.offset_y;
            }
            for (auto& line : result.lines) {
                line.y += result.offset_y;
            }
        }

        result.width = static_cast<uint32_t>(std::lround(result.box_width));
        result.height = static_cast<uint32_t>(std::lround(result.box_height));

        // Populate unified fields
        result.total_bounds = {};
        for (std::size_t i = 0; i < result.lines.size(); ++i) {
            auto& line = result.lines[i];
            const std::size_t end = std::min(result.glyphs.size(), line.glyph_start_index + line.glyph_count);
            
            math::RectF line_bounds{};
            bool have_bounds = false;
            
            for (std::size_t g_idx = line.glyph_start_index; g_idx < end; ++g_idx) {
                auto& g = result.glyphs[g_idx];
                g.line_index = i;
                if (!have_bounds) {
                    line_bounds = g.bounds;
                    have_bounds = true;
                } else {
                    line_bounds = union_rects(line_bounds, g.bounds);
                }
                
                if (rect_is_empty(result.total_bounds)) {
                    result.total_bounds = g.bounds;
                } else {
                    result.total_bounds = union_rects(result.total_bounds, g.bounds);
                }
            }
            line.bounds = line_bounds;
            line.ascent = static_cast<float>(primary_font.ascent());
            line.descent = static_cast<float>(primary_font.descent());
        }

        // Build runs
        if (!result.glyphs.empty()) {
            std::size_t run_start = 0;
            const Font* run_font = result.glyphs.front().resolved_font;
            math::RectF run_bounds = result.glyphs.front().bounds;
            
            for (std::size_t i = 1; i <= result.glyphs.size(); ++i) {
                const bool run_break = i == result.glyphs.size() || result.glyphs[i].resolved_font != run_font;
                if (i < result.glyphs.size()) {
                    run_bounds = union_rects(run_bounds, result.glyphs[i].bounds);
                }
                
                if (run_break) {
                    TextRun run;
                    run.start_glyph_index = run_start;
                    run.length = i - run_start;
                    run.font = run_font;
                    run.font_size = static_cast<float>(style.pixel_size);
                    run.bounds = run_bounds;
                    result.runs.push_back(run);
                    
                    if (i < result.glyphs.size()) {
                        run_start = i;
                        run_font = result.glyphs[i].resolved_font;
                        run_bounds = result.glyphs[i].bounds;
                    }
                }
            }
        }

        // Clusters
        std::map<std::size_t, std::vector<std::size_t>> cluster_map;
        for (std::size_t i = 0; i < result.glyphs.size(); ++i) {
            cluster_map[result.glyphs[i].cluster_index].push_back(i);
        }
        for (const auto& [idx, g_indices] : cluster_map) {
            if (g_indices.empty()) continue;
            const auto& first = result.glyphs[g_indices.front()];
            GlyphCluster c;
            c.source_text_start = first.cluster_codepoint_start;
            c.source_text_length = first.cluster_codepoint_count;
            c.glyph_start = g_indices.front();
            c.glyph_count = g_indices.size();
            result.clusters.push_back(c);
        }

        return result;
    }
};

TextLayoutResult layout_text(const BitmapFont& font, std::string_view utf8_text, const TextStyle& style, const TextBoxSpec& text_box, const TextLayoutOptions& options) {
    if (!font.is_loaded()) return {};

    LayoutCacheKey key;
    key.text = std::string(utf8_text);
    key.font_id = static_cast<const tachyon::text::Font&>(font).font_id();
    key.pixel_size = style.pixel_size;
    key.box = text_box;
    key.options = options;

    TextLayoutResult result;
    if (LayoutCache::get_instance().get(key, result)) {
        return result;
    }

    std::vector<const Font*> chain = { &font };
    result = InternalLayoutEngine::layout(chain, utf8_text, style, text_box, options);
    
    LayoutCache::get_instance().put(key, result);
    return result;
}

TextLayoutResult layout_text(
    const FontRegistry& registry,
    const std::string& font_name,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBoxSpec& text_box,
    const TextLayoutOptions& options) {

    auto fallback_chain = registry.get_fallback_chain(font_name);
    if (fallback_chain.empty()) {
        const Font* default_font = registry.default_font();
        if (default_font) {
            fallback_chain = {default_font};
        } else {
            return {};
        }
    }
    return InternalLayoutEngine::layout(fallback_chain, utf8_text, style, text_box, options);
}

TextLayoutResult layout_text(const BitmapFont& font, std::string_view utf8_text, const TextStyle& style, const TextBox& text_box, TextAlignment alignment, const TextLayoutOptions& options) {
    if (!font.is_loaded()) return {};
    std::vector<const Font*> chain = { &font };
    TextBoxSpec spec;
    spec.mode = (text_box.width > 0 || text_box.height > 0) ? TextBoxMode::Fixed : TextBoxMode::Auto;
    spec.width = static_cast<float>(text_box.width);
    spec.height = static_cast<float>(text_box.height);
    spec.horizontal_align = alignment;
    spec.vertical_align = VerticalAlign::Top;
    return InternalLayoutEngine::layout(chain, utf8_text, style, spec, options);
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
        const Font* default_font = registry.default_font();
        if (default_font) {
            fallback_chain = {default_font};
        } else {
            return {};
        }
    }
    TextBoxSpec spec;
    spec.mode = (text_box.width > 0 || text_box.height > 0) ? TextBoxMode::Fixed : TextBoxMode::Auto;
    spec.width = static_cast<float>(text_box.width);
    spec.height = static_cast<float>(text_box.height);
    spec.horizontal_align = alignment;
    spec.vertical_align = VerticalAlign::Top;
    return InternalLayoutEngine::layout(fallback_chain, utf8_text, style, spec, options);
}

TextRasterSurface rasterize_layout_debug(const TextLayoutResult& layout) {
    TextRasterSurface surface(layout.width, layout.height);
    if (layout.width == 0 || layout.height == 0) return surface;

    for (const auto& line : layout.lines) {
        surface.draw_line(0, static_cast<int>(std::lround(line.y)), layout.width, static_cast<int>(std::lround(line.y)), renderer2d::Color{0.0f, 1.0f, 0.0f, 0.5f});
        surface.draw_line(0, static_cast<int>(std::lround(line.y + static_cast<float>(layout.line_height))), layout.width, static_cast<int>(std::lround(line.y + static_cast<float>(layout.line_height))), renderer2d::Color{1.0f, 0.0f, 0.0f, 0.2f});
    }

    for (const auto& g : layout.glyphs) {
        surface.draw_rect(static_cast<int>(std::lround(g.position.x)), static_cast<int>(std::lround(g.position.y)), static_cast<int>(std::lround(g.width)), static_cast<int>(std::lround(g.height)), renderer2d::Color{1.0f, 1.0f, 1.0f, 0.3f});
        renderer2d::Color cluster_color = (g.cluster_index % 2 == 0) ? renderer2d::Color{1.0f, 1.0f, 0.0f, 0.8f} : renderer2d::Color{0.0f, 1.0f, 1.0f, 0.8f};
        surface.draw_line(static_cast<int>(std::lround(g.position.x)), static_cast<int>(std::lround(g.position.y + g.height - 2.0f)), static_cast<int>(std::lround(g.position.x + g.width)), static_cast<int>(std::lround(g.position.y + g.height - 2.0f)), cluster_color);
    }

    return surface;
}

} // namespace tachyon::text
