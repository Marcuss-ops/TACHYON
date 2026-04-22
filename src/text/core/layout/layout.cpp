#include "tachyon/text/layout/layout.h"
#include "tachyon/text/animation/text_animator_utils.h"

#include <algorithm>
#include <cmath>
#include <span>
#include <string_view>

namespace tachyon::text {

namespace {

template <typename BlendFn>
void fill_rect(std::uint32_t surface_width, std::uint32_t surface_height, int x, int y, int width, int height, BlendFn&& blend_fn) {
    if (width <= 0 || height <= 0 || surface_width == 0U || surface_height == 0U) return;
    const int x0 = std::max(0, x), y0 = std::max(0, y), x1 = std::min<int>(surface_width, x + width), y1 = std::min<int>(surface_height, y + height);
    for (int py = y0; py < y1; ++py)
        for (int px = x0; px < x1; ++px)
            blend_fn(static_cast<std::uint32_t>(px), static_cast<std::uint32_t>(py));
}

struct AnimatorSample {
    float offset_x{0.0f};
    float offset_y{0.0f};
    float scale{1.0f};
    float opacity{1.0f};
};

AnimatorSample sample_animator(
    const TextAnimatorSpec& animator,
    const TextLayoutResult& layout,
    const PositionedGlyph& glyph,
    float time_seconds) {

    AnimatorSample sample;
    const float coverage = compute_coverage(animator.selector, glyph.glyph_index, layout.glyphs.size());
    if (coverage <= 0.0f) {
        return sample;
    }

    if (animator.properties.position_offset_value.has_value() || !animator.properties.position_offset_keyframes.empty()) {
        const math::Vector2 offset = sample_vector2_kfs(animator.properties.position_offset_value, animator.properties.position_offset_keyframes, time_seconds);
        sample.offset_x += offset.x * coverage;
        sample.offset_y += offset.y * coverage;
    }

    if (animator.properties.scale_value.has_value() || !animator.properties.scale_keyframes.empty()) {
        const double scale_value = sample_scalar_kfs(animator.properties.scale_value, animator.properties.scale_keyframes, time_seconds);
        const float scale_mul = static_cast<float>(std::max(0.05, scale_value));
        sample.scale *= 1.0f + (scale_mul - 1.0f) * coverage;
    }

    if (animator.properties.opacity_value.has_value() || !animator.properties.opacity_keyframes.empty()) {
        const double opacity_value = sample_scalar_kfs(animator.properties.opacity_value, animator.properties.opacity_keyframes, time_seconds);
        const float opacity_mul = static_cast<float>(std::clamp(opacity_value, 0.0, 1.0));
        sample.opacity *= 1.0f + (opacity_mul - 1.0f) * coverage;
    }

    return sample;
}

} // namespace

TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    const TextLayoutOptions& layout_options,
    const TextAnimationOptions& animation) {

    const TextLayoutResult layout = layout_text(font, utf8_text, style, text_box, alignment, layout_options);
    TextRasterSurface surface(layout.width, layout.height);
    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) return surface;

    for (const PositionedGlyph& positioned : layout.glyphs) {
        const GlyphBitmap* glyph = font.has_freetype_face() ? font.find_glyph_by_index(positioned.font_glyph_index) : font.find_scaled_glyph(positioned.codepoint, layout.scale);
        if (!glyph || glyph->width == 0U || glyph->height == 0U) continue;

        float ox = 0.0f, oy = 0.0f, sc = 1.0f, op = 1.0f;
        if (animation.enabled) {
            const float phase_seconds = animation.time_seconds - static_cast<float>(positioned.glyph_index) * 0.1f;
            const float wave_period = std::max(0.001f, animation.wave_period_seconds);
            const float wave_phase = (phase_seconds / wave_period) * 6.28318530717958647692f;

            ox = animation.per_glyph_offset_x * static_cast<float>(positioned.glyph_index) + std::sin(wave_phase) * animation.wave_amplitude_x;
            oy = animation.per_glyph_offset_y * static_cast<float>(positioned.glyph_index) + std::cos(wave_phase) * animation.wave_amplitude_y;
            sc = std::max(0.05f, 1.0f + animation.per_glyph_scale_delta * static_cast<float>(positioned.glyph_index));
            op = std::clamp(1.0f - animation.per_glyph_opacity_drop * static_cast<float>(positioned.glyph_index), 0.0f, 1.0f);
        }

        renderer2d::Color color = style.fill_color; color.a *= op;
        const int dx = positioned.x + (int)std::lround(ox), dy = positioned.y + (int)std::lround(oy);
        surface.render_glyph(*glyph, dx, dy, (int)std::lround((float)glyph->width * layout.scale * sc), (int)std::lround((float)glyph->height * layout.scale * sc), color);
    }
    return surface;
}

TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    float time_seconds,
    std::span<const TextAnimatorSpec> animators,
    const TextLayoutOptions& layout_options) {

    const TextLayoutResult layout = layout_text(font, utf8_text, style, text_box, alignment, layout_options);
    TextRasterSurface surface(layout.width, layout.height);
    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) {
        return surface;
    }

    for (const PositionedGlyph& positioned : layout.glyphs) {
        const GlyphBitmap* glyph = font.has_freetype_face()
            ? font.find_glyph_by_index(positioned.font_glyph_index)
            : font.find_scaled_glyph(positioned.codepoint, layout.scale);
        if (!glyph || glyph->width == 0U || glyph->height == 0U) {
            continue;
        }

        float offset_x = 0.0f;
        float offset_y = 0.0f;
        float scale = 1.0f;
        float opacity = 1.0f;

        for (const auto& animator : animators) {
            const AnimatorSample sample = sample_animator(animator, layout, positioned, time_seconds);
            offset_x += sample.offset_x;
            offset_y += sample.offset_y;
            scale *= sample.scale;
            opacity *= sample.opacity;
        }

        renderer2d::Color color = style.fill_color;
        color.a *= std::clamp(opacity, 0.0f, 1.0f);

        const int dx = positioned.x + static_cast<int>(std::lround(offset_x));
        const int dy = positioned.y + static_cast<int>(std::lround(offset_y));
        const int dw = std::max(1, static_cast<int>(std::lround(static_cast<float>(glyph->width) * layout.scale * scale)));
        const int dh = std::max(1, static_cast<int>(std::lround(static_cast<float>(glyph->height) * layout.scale * scale)));
        surface.render_glyph(*glyph, dx, dy, dw, dh, color);
    }

    return surface;
}

TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    std::span<const TextHighlightSpan> highlights,
    const TextLayoutOptions& layout_options,
    const TextAnimationOptions& animation) {

    const TextLayoutResult layout = layout_text(font, utf8_text, style, text_box, alignment, layout_options);
    TextRasterSurface surface(layout.width, layout.height);
    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) return surface;

    for (const auto& span : highlights) {
        if (span.start_glyph >= layout.glyphs.size()) continue;
        const auto end = std::min<size_t>(span.end_glyph, layout.glyphs.size());
        for (size_t i = span.start_glyph; i < end; ++i) {
            const auto& g = layout.glyphs[i];
            fill_rect(layout.width, layout.height, g.x - (int)std::lround(span.padding_x), g.y - (int)std::lround(span.padding_y), g.width + (int)std::lround(span.padding_x * 2), g.height + (int)std::lround(span.padding_y * 2), [&](uint32_t px, uint32_t py) {
                surface.blend_pixel(px, py, span.color, 255);
            });
        }
    }

    for (const PositionedGlyph& positioned : layout.glyphs) {
        const GlyphBitmap* glyph = font.has_freetype_face() ? font.find_glyph_by_index(positioned.font_glyph_index) : font.find_scaled_glyph(positioned.codepoint, layout.scale);
        if (!glyph || glyph->width == 0U || glyph->height == 0U) continue;
        float ox = 0.0f, oy = 0.0f, sc = 1.0f, op = 1.0f;
        if (animation.enabled) {
            const float phase_seconds = animation.time_seconds - static_cast<float>(positioned.glyph_index) * 0.1f;
            const float wave_period = std::max(0.001f, animation.wave_period_seconds);
            const float wave_phase = (phase_seconds / wave_period) * 6.28318530717958647692f;
            ox = animation.per_glyph_offset_x * static_cast<float>(positioned.glyph_index) + std::sin(wave_phase) * animation.wave_amplitude_x;
            oy = animation.per_glyph_offset_y * static_cast<float>(positioned.glyph_index) + std::cos(wave_phase) * animation.wave_amplitude_y;
            sc = std::max(0.05f, 1.0f + animation.per_glyph_scale_delta * static_cast<float>(positioned.glyph_index));
            op = std::clamp(1.0f - animation.per_glyph_opacity_drop * static_cast<float>(positioned.glyph_index), 0.0f, 1.0f);
        }
        renderer2d::Color color = style.fill_color; color.a *= op;
        surface.render_glyph(*glyph, positioned.x + (int)std::lround(ox), positioned.y + (int)std::lround(oy), (int)std::lround((float)glyph->width * layout.scale * sc), (int)std::lround((float)glyph->height * layout.scale * sc), color);
    }
    return surface;
}

TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    const TextOutlineOptions& outline) {
    
    (void)outline;
    TextLayoutResult layout = layout_text(font, utf8_text, style, text_box, alignment);
    TextRasterSurface surface(layout.width, layout.height);
    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) return surface;

    for (const PositionedGlyph& positioned : layout.glyphs) {
        const GlyphBitmap* glyph = font.has_freetype_face() ? font.find_glyph_by_index(positioned.font_glyph_index) : font.find_scaled_glyph(positioned.codepoint, layout.scale);
        if (!glyph || glyph->width == 0U || glyph->height == 0U) continue;
        surface.render_glyph(*glyph, positioned.x, positioned.y, (int)std::lround((float)glyph->width * layout.scale), (int)std::lround((float)glyph->height * layout.scale), style.fill_color);
    }
    return surface;
}

} // namespace tachyon::text
