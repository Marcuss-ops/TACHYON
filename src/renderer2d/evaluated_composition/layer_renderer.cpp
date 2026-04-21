#include "tachyon/renderer2d/evaluated_composition/layer_renderer.h"
#include "tachyon/renderer2d/color_transfer.h"
#include "tachyon/renderer2d/rasterizer_ops.h"
#include "tachyon/renderer2d/texture_resolver.h"
#include "tachyon/text/font.h"
#include "tachyon/text/layout.h"
#include "tachyon/text/subtitle.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <span>

namespace tachyon::renderer2d {

using renderer2d::Color;

namespace {

Color from_spec(const ColorSpec& spec) {
    return detail::sRGB_to_Linear(Color{
        static_cast<float>(spec.r) / 255.0f,
        static_cast<float>(spec.g) / 255.0f,
        static_cast<float>(spec.b) / 255.0f,
        static_cast<float>(spec.a) / 255.0f
    });
}

Color color_with_opacity(Color color, float opacity) {
    color.a *= std::clamp(opacity, 0.0f, 1.0f);
    return color;
}

struct RectI {
    int x, y, width, height;
};

RectI layer_bounds(const scene::EvaluatedLayerState& layer, std::int64_t comp_w, std::int64_t comp_h) {
    int base_width = static_cast<int>(layer.width);
    int base_height = static_cast<int>(layer.height);
    if (base_width <= 0 || base_height <= 0) {
        if (layer.type == scene::LayerType::Text) { base_width = std::max(64, static_cast<int>(comp_w / 6)); base_height = std::max(32, static_cast<int>(comp_h / 10)); }
        else if (layer.type == scene::LayerType::Solid) { base_width = std::max(64, static_cast<int>(comp_w / 4)); base_height = std::max(32, static_cast<int>(comp_h / 8)); }
        else { base_width = 100; base_height = 100; }
    }
    const int w = std::max(1, static_cast<int>(std::round(static_cast<float>(base_width) * std::abs(layer.local_transform.scale.x))));
    const int h = std::max(1, static_cast<int>(std::round(static_cast<float>(base_height) * std::abs(layer.local_transform.scale.y))));
    return RectI{ static_cast<int>(std::round(layer.local_transform.position.x)) - w / 2, static_cast<int>(std::round(layer.local_transform.position.y)) - h / 2, w, h };
}

renderer2d::Color from_spec_color(const std::optional<ColorSpec>& spec, renderer2d::Color fallback) {
    if (!spec.has_value()) {
        return fallback;
    }
    return from_spec(*spec);
}

void blend_text_surface(
    const ::tachyon::text::TextRasterSurface& surface,
    int origin_x,
    int origin_y,
    std::vector<float>& accum_r,
    std::vector<float>& accum_g,
    std::vector<float>& accum_b,
    std::vector<float>& accum_a,
    std::int64_t width) {

    const std::uint32_t surf_width = surface.width();
    const std::uint32_t surf_height = surface.height();
    const auto& pixels = surface.rgba_pixels();

    for (std::uint32_t y = 0; y < surf_height; ++y) {
        for (std::uint32_t x = 0; x < surf_width; ++x) {
            const std::int64_t dst_x = static_cast<std::int64_t>(origin_x) + static_cast<std::int64_t>(x);
            const std::int64_t dst_y = static_cast<std::int64_t>(origin_y) + static_cast<std::int64_t>(y);
            if (dst_x < 0 || dst_y < 0) {
                continue;
            }

            const std::size_t index = static_cast<std::size_t>(dst_y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(dst_x);
            if (index >= accum_a.size()) {
                continue;
            }

            const std::size_t src_index = (static_cast<std::size_t>(y) * surf_width + x) * 4U;
            const float src_r = static_cast<float>(pixels[src_index + 0U]) / 255.0f;
            const float src_g = static_cast<float>(pixels[src_index + 1U]) / 255.0f;
            const float src_b = static_cast<float>(pixels[src_index + 2U]) / 255.0f;
            const float src_a = static_cast<float>(pixels[src_index + 3U]) / 255.0f;
            if (src_a <= 0.0f) {
                continue;
            }

            const float dst_a = accum_a[index];
            const float out_a = src_a + dst_a * (1.0f - src_a);
            const float src_r_p = src_r * src_a;
            const float src_g_p = src_g * src_a;
            const float src_b_p = src_b * src_a;
            const float dst_r_p = accum_r[index] * dst_a;
            const float dst_g_p = accum_g[index] * dst_a;
            const float dst_b_p = accum_b[index] * dst_a;

            if (out_a <= 0.0f) {
                accum_r[index] = 0.0f;
                accum_g[index] = 0.0f;
                accum_b[index] = 0.0f;
                accum_a[index] = 0.0f;
                continue;
            }

            accum_r[index] = (src_r_p + dst_r_p * (1.0f - src_a)) / out_a;
            accum_g[index] = (src_g_p + dst_g_p * (1.0f - src_a)) / out_a;
            accum_b[index] = (src_b_p + dst_b_p * (1.0f - src_a)) / out_a;
            accum_a[index] = out_a;
        }
    }
}

const std::vector<::tachyon::text::SubtitleEntry>* load_subtitle_entries(const std::filesystem::path& path) {
    static std::unordered_map<std::string, std::vector<::tachyon::text::SubtitleEntry>> cache;
    const std::string key = path.string();
    const auto it = cache.find(key);
    if (it != cache.end()) {
        return &it->second;
    }

    const auto parsed = ::tachyon::text::parse_srt(path);
    if (!parsed.value.has_value()) {
        return nullptr;
    }

    auto [inserted_it, _] = cache.emplace(key, std::move(*parsed.value));
    return &inserted_it->second;
}

} // namespace

void LayerRenderer::renderLayer(
    const scene::EvaluatedLayerState& layer_state,
    const scene::EvaluatedCompositionState& comp_state,
    RenderContext2D& context,
    std::vector<float>& accum_r, std::vector<float>& accum_g, std::vector<float>& accum_b, std::vector<float>& accum_a) {

    switch (layer_state.type) {
    case scene::LayerType::Solid: renderSolidLayer(layer_state, comp_state, context, accum_r, accum_g, accum_b, accum_a); break;
    case scene::LayerType::Image: renderImageLayer(layer_state, comp_state, context, accum_r, accum_g, accum_b, accum_a); break;
    case scene::LayerType::Text: renderTextLayer(layer_state, comp_state, context, accum_r, accum_g, accum_b, accum_a); break;
    case scene::LayerType::Shape: renderShapeLayer(layer_state, comp_state, context, accum_r, accum_g, accum_b, accum_a); break;
    default: break;
    }
}

void LayerRenderer::renderSolidLayer(
    const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& comp,
    RenderContext2D& context,
    std::vector<float>& r, std::vector<float>& g, std::vector<float>& b, std::vector<float>& a) {
    (void)layer; (void)comp; (void)context; (void)r; (void)g; (void)b; (void)a;
    // Basic solid fill logic would go here, updating accumulation buffers
}

void LayerRenderer::renderShapeLayer(
    const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& comp,
    RenderContext2D& context,
    std::vector<float>& r, std::vector<float>& g, std::vector<float>& b, std::vector<float>& a) {
    (void)comp; (void)context; (void)layer; (void)r; (void)g; (void)b; (void)a;
}

void LayerRenderer::renderImageLayer(
    const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& comp,
    RenderContext2D& context,
    std::vector<float>& r, std::vector<float>& g, std::vector<float>& b, std::vector<float>& a) {
    (void)comp; (void)context; (void)layer; (void)r; (void)g; (void)b; (void)a;
}

void LayerRenderer::renderTextLayer(
    const scene::EvaluatedLayerState& layer, const scene::EvaluatedCompositionState& comp,
    RenderContext2D& context,
    std::vector<float>& r, std::vector<float>& g, std::vector<float>& b, std::vector<float>& a) {
    (void)context;
    const bool has_text_content = !layer.text_content.empty();
    const bool has_subtitle = layer.subtitle_path.has_value();
    if (!has_text_content && !has_subtitle) {
        return;
    }

    const auto* font = context.font;
    if (font == nullptr || !font->is_loaded()) {
        return;
    }

    ::tachyon::text::TextStyle style;
    style.pixel_size = static_cast<std::uint32_t>(std::max<std::int32_t>(1, static_cast<std::int32_t>(std::lround(layer.font_size))));
    style.fill_color = from_spec(layer.fill_color);

    const ::tachyon::text::TextBox text_box{
        static_cast<std::uint32_t>(std::max<std::int64_t>(1, comp.width)),
        static_cast<std::uint32_t>(std::max<std::int64_t>(1, comp.height / 4)),
        true
    };

    std::string text = layer.text_content;
    ::tachyon::text::TextAlignment alignment = ::tachyon::text::TextAlignment::Center;
    if (layer.text_alignment == 0) {
        alignment = ::tachyon::text::TextAlignment::Left;
    } else if (layer.text_alignment == 2) {
        alignment = ::tachyon::text::TextAlignment::Right;
    }
    bool use_outline = false;
    ::tachyon::text::TextOutlineOptions outline{
        layer.subtitle_outline_width,
        from_spec_color(layer.subtitle_outline_color, renderer2d::Color::black())
    };

    if (!has_text_content && has_subtitle) {
        const auto* entries = load_subtitle_entries(*layer.subtitle_path);
        if (entries == nullptr) {
            return;
        }

        const ::tachyon::text::SubtitleEntry* active = ::tachyon::text::find_active_subtitle(*entries, comp.composition_time_seconds);
        if (active == nullptr || active->text.empty()) {
            return;
        }

        text = active->text;
        use_outline = layer.subtitle_outline_width > 0.0f || layer.subtitle_outline_color.has_value();
    }

    std::vector<::tachyon::text::TextHighlightSpan> highlight_spans;
    highlight_spans.reserve(layer.text_highlights.size());
    for (const auto& highlight : layer.text_highlights) {
        if (highlight.end_glyph <= highlight.start_glyph) {
            continue;
        }
        ::tachyon::text::TextHighlightSpan span;
        span.start_glyph = highlight.start_glyph;
        span.end_glyph = highlight.end_glyph;
        span.color = from_spec(highlight.color);
        span.padding_x = highlight.padding_x;
        span.padding_y = highlight.padding_y;
        highlight_spans.push_back(span);
    }

    const bool can_apply_highlights = !highlight_spans.empty();
    ::tachyon::text::TextRasterSurface subtitle_surface =
        can_apply_highlights
            ? ::tachyon::text::rasterize_text_rgba(
                *font,
                text,
                style,
                text_box,
                alignment,
                std::span<const ::tachyon::text::TextHighlightSpan>(highlight_spans.data(), highlight_spans.size()),
                ::tachyon::text::TextLayoutOptions{},
                ::tachyon::text::TextAnimationOptions{})
            : (use_outline
                ? ::tachyon::text::rasterize_text_rgba(
                    *font,
                    text,
                    style,
                    text_box,
                    alignment,
                    outline)
                : ::tachyon::text::rasterize_text_rgba(
                    *font,
                    text,
                    style,
                    text_box,
                    alignment));

    if (subtitle_surface.width() == 0U || subtitle_surface.height() == 0U) {
        return;
    }

    const int origin_x = has_text_content
        ? (alignment == ::tachyon::text::TextAlignment::Left
            ? std::max(0, static_cast<int>(std::lround(layer.local_transform.position.x)))
            : alignment == ::tachyon::text::TextAlignment::Right
                ? std::max(0, static_cast<int>(std::lround(layer.local_transform.position.x)) - static_cast<int>(subtitle_surface.width()))
                : std::max(0, static_cast<int>(std::lround(layer.local_transform.position.x)) - static_cast<int>(subtitle_surface.width() / 2U)))
        : std::max(0, static_cast<int>((comp.width - static_cast<std::int64_t>(subtitle_surface.width())) / 2));
    const int origin_y = has_text_content
        ? std::max(0, static_cast<int>(std::lround(layer.local_transform.position.y)) - static_cast<int>(subtitle_surface.height() / 2U))
        : std::max<int>(0, static_cast<int>(comp.height - static_cast<std::int64_t>(subtitle_surface.height()) - std::max<std::int64_t>(8, static_cast<std::int64_t>(font->line_height() / 2))));
    blend_text_surface(subtitle_surface, origin_x, origin_y, r, g, b, a, comp.width);
}

} // namespace tachyon::renderer2d
