#include "layout_engine_helpers.h"
#include "tachyon/text/i18n/bidi_engine.h"
#include "tachyon/renderer2d/text/utf8/utf8_decoder.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>
#if __cplusplus >= 201703L
#include <execution>
#include <numeric>
#endif

namespace tachyon::text {

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

        LayoutCacheKey layout_cache_key;
        layout_cache_key.text = std::string(utf8_text);
        layout_cache_key.font_id = fallback_chain.front()->id();
        layout_cache_key.pixel_size = style.pixel_size;
        layout_cache_key.features = style.features;
        layout_cache_key.box_width = text_box.width;
        layout_cache_key.box_height = text_box.height;
        layout_cache_key.multiline = text_box.multiline;
        layout_cache_key.alignment = alignment;
        layout_cache_key.tracking = options.tracking;
        layout_cache_key.word_wrap = options.word_wrap;
        layout_cache_key.use_sdf = options.use_sdf;

        auto& layout_cache = LayoutCache::get_instance();
        if (layout_cache.get(layout_cache_key, result)) {
            return result;
        }

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
                    subruns.push_back({selected_font, {}, run.direction, current_cluster->start_index});
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

            std::vector<ShapedGlyphRun> shaped_runs(subruns.size());
#pragma omp parallel for
            for (int idx = 0; idx < (int)subruns.size(); ++idx) {
                const auto& sub = subruns[idx];
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
                shaped_runs[idx] = std::move(shaped);
            }

            for (size_t idx = 0; idx < subruns.size(); ++idx) {
                const auto& sub = subruns[idx];
                const auto& shaped = shaped_runs[idx];
                bool contains_break = false;
                for (std::uint32_t cp : sub.codepoints) {
                    if (is_breakable_space(cp) || cp == '-' || (cp >= 0x4E00 && cp <= 0x9FFF)) {
                        contains_break = true;
                        break;
                    }
                }

                if (wrap_width > 0 && text_box.multiline && options.word_wrap && pen_x > 0 && std::abs(shaped.width) > 0 && (pen_x + std::abs(shaped.width)) > wrap_width) {
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
        sync_resolved_layout(result, primary_font, style);
        
        layout_cache.put(layout_cache_key, result);
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

    for (const auto& line : layout.lines) {
        surface.draw_line(0, line.y, layout.width, line.y, renderer2d::Color{0.0f, 1.0f, 0.0f, 0.5f});
        surface.draw_line(0, line.y + layout.line_height, layout.width, line.y + layout.line_height, renderer2d::Color{1.0f, 0.0f, 0.0f, 0.2f});
    }

    for (const auto& g : layout.glyphs) {
        surface.draw_rect(g.x, g.y, g.width, g.height, renderer2d::Color{1.0f, 1.0f, 1.0f, 0.3f});
        renderer2d::Color cluster_color = (g.cluster_index % 2 == 0) ? renderer2d::Color{1.0f, 1.0f, 0.0f, 0.8f} : renderer2d::Color{0.0f, 1.0f, 1.0f, 0.8f};
        surface.draw_line(g.x, g.y + g.height - 2, g.x + g.width, g.y + g.height - 2, cluster_color);
    }

    return surface;
}

} // namespace tachyon::text
