#include "tachyon/text/layout/layout.h"
#include "tachyon/text/animation/text_animator_utils.h"

#include <algorithm>
#include <cmath>
#include <span>
#include <string_view>

namespace tachyon::text {

namespace {
constexpr float TWO_PI = 6.28318530717958647692f;
} // namespace

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
    TextAnimatorContext ctx;
    ctx.glyph_index = glyph.glyph_index;
    ctx.total_glyphs = static_cast<float>(layout.glyphs.size());
    ctx.time = time_seconds;

    const float coverage = compute_coverage(animator.selector, ctx);
    if (coverage <= 0.0f) {
        return sample;
    }

    if (animator.properties.position_offset_value.has_value() || !animator.properties.position_offset_keyframes.empty()) {
        const ::tachyon::math::Vector2 offset = sample_vector2_kfs(animator.properties.position_offset_value, animator.properties.position_offset_keyframes, time_seconds);
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
    const TextBoxSpec& text_box,
    const TextLayoutOptions& layout_options,
    const TextAnimationOptions& animation) {

    const TextLayoutResult layout = layout_text(font, utf8_text, style, text_box, layout_options);
    TextRasterSurface surface(layout.width, layout.height);
    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) return surface;

    for (const PositionedGlyph& positioned : layout.glyphs) {
        const GlyphBitmap* glyph = font.has_freetype_face() ? font.find_glyph_by_index(positioned.font_glyph_index) : font.find_scaled_glyph(positioned.codepoint, layout.scale);
        if (!glyph || glyph->width == 0U || glyph->height == 0U) continue;

        float ox = 0.0f, oy = 0.0f, sc = 1.0f, op = 1.0f;
        if (animation.enabled && !animation.animators.empty()) {
            for (const auto& animator : animation.animators) {
                const AnimatorSample sample = sample_animator(animator, layout, positioned, animation.time_seconds);
                ox += sample.offset_x;
                oy += sample.offset_y;
                sc *= sample.scale;
                op *= sample.opacity;
            }
        }

        renderer2d::Color color = style.fill_color;
        // Apply gradient if specified
        if (style.gradient.has_value()) {
            const auto& grad = *style.gradient;
            const float t = static_cast<float>(positioned.position.y) / static_cast<float>(std::max(1U, layout.height));
            // Simple 2-stop gradient sampling
            if (grad.stops.size() >= 2) {
                const float clamped_t = std::clamp(t, 0.0f, 1.0f);
                // Find the two nearest stops
                for (size_t i = 0; i < grad.stops.size() - 1; ++i) {
                    if (clamped_t >= grad.stops[i].position && clamped_t <= grad.stops[i + 1].position) {
                        const float local_t = (clamped_t - grad.stops[i].position) / (grad.stops[i + 1].position - grad.stops[i].position);
                        color.r = grad.stops[i].color.r + (grad.stops[i + 1].color.r - grad.stops[i].color.r) * local_t;
                        color.g = grad.stops[i].color.g + (grad.stops[i + 1].color.g - grad.stops[i].color.g) * local_t;
                        color.b = grad.stops[i].color.b + (grad.stops[i + 1].color.b - grad.stops[i].color.b) * local_t;
                        break;
                    }
                }
            } else if (!grad.stops.empty()) {
                color = grad.stops[0].color;
            }
        }
        color.a *= op;
        const int dx = static_cast<int>(std::lround(positioned.position.x + ox));
        const int dy = static_cast<int>(std::lround(positioned.position.y + oy));
        const int dw = std::max(1, static_cast<int>(std::lround(static_cast<float>(glyph->width) * layout.scale * sc)));
        const int dh = std::max(1, static_cast<int>(std::lround(static_cast<float>(glyph->height) * layout.scale * sc)));
        surface.render_glyph(*glyph, dx, dy, dw, dh, color);
    }

    // Apply shadow and glow effects
    if (style.glow.enabled) {
        surface.apply_glow(style.glow);
    }
    if (style.shadow.enabled) {
        surface.apply_shadow(style.shadow);
    }

    return surface;
}

TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBoxSpec& text_box,
    float time_seconds,
    std::span<const TextAnimatorSpec> animators,
    const TextLayoutOptions& layout_options) {

    const TextLayoutResult layout = layout_text(font, utf8_text, style, text_box, layout_options);
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
        // Apply gradient if specified
        if (style.gradient.has_value()) {
            const auto& grad = *style.gradient;
            const float t = static_cast<float>(positioned.position.y) / static_cast<float>(std::max(1U, layout.height));
            if (grad.stops.size() >= 2) {
                const float clamped_t = std::clamp(t, 0.0f, 1.0f);
                for (size_t i = 0; i < grad.stops.size() - 1; ++i) {
                    if (clamped_t >= grad.stops[i].position && clamped_t <= grad.stops[i + 1].position) {
                        const float local_t = (clamped_t - grad.stops[i].position) / (grad.stops[i + 1].position - grad.stops[i].position);
                        color.r = grad.stops[i].color.r + (grad.stops[i + 1].color.r - grad.stops[i].color.r) * local_t;
                        color.g = grad.stops[i].color.g + (grad.stops[i + 1].color.g - grad.stops[i].color.g) * local_t;
                        color.b = grad.stops[i].color.b + (grad.stops[i + 1].color.b - grad.stops[i].color.b) * local_t;
                        break;
                    }
                }
            } else if (!grad.stops.empty()) {
                color = grad.stops[0].color;
            }
        }
        color.a *= std::clamp(opacity, 0.0f, 1.0f);

        const int dx = static_cast<int>(std::lround(positioned.position.x + offset_x));
        const int dy = static_cast<int>(std::lround(positioned.position.y + offset_y));
        const int dw = std::max(1, static_cast<int>(std::lround(static_cast<float>(glyph->width) * layout.scale * scale)));
        const int dh = std::max(1, static_cast<int>(std::lround(static_cast<float>(glyph->height) * layout.scale * scale)));
        surface.render_glyph(*glyph, dx, dy, dw, dh, color);
    }

    // Apply shadow and glow effects
    if (style.glow.enabled) {
        surface.apply_glow(style.glow);
    }
    if (style.shadow.enabled) {
        surface.apply_shadow(style.shadow);
    }

    return surface;
}

TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBoxSpec& text_box,
    std::span<const TextHighlightSpan> highlights,
    const TextLayoutOptions& layout_options,
    const TextAnimationOptions& animation) {

    const TextLayoutResult layout = layout_text(font, utf8_text, style, text_box, layout_options);
    TextRasterSurface surface(layout.width, layout.height);
    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) return surface;

    for (const auto& span : highlights) {
        if (span.start_glyph >= layout.glyphs.size()) continue;
        const auto end = std::min<size_t>(span.end_glyph, layout.glyphs.size());
        for (size_t i = span.start_glyph; i < end; ++i) {
            const auto& g = layout.glyphs[i];
            const int gx = static_cast<int>(std::lround(g.position.x));
            const int gy = static_cast<int>(std::lround(g.position.y));
            const int gw = static_cast<int>(std::lround(g.width));
            const int gh = static_cast<int>(std::lround(g.height));
            fill_rect(layout.width, layout.height, gx - (int)std::lround(span.padding_x), gy - (int)std::lround(span.padding_y), gw + (int)std::lround(span.padding_x * 2), gh + (int)std::lround(span.padding_y * 2), [&](uint32_t px, uint32_t py) {
                surface.blend_pixel(px, py, span.color, 255);
            });
        }
    }

    for (const PositionedGlyph& positioned : layout.glyphs) {
        const GlyphBitmap* glyph = font.has_freetype_face() ? font.find_glyph_by_index(positioned.font_glyph_index) : font.find_scaled_glyph(positioned.codepoint, layout.scale);
        if (!glyph || glyph->width == 0U || glyph->height == 0U) continue;

        float ox = 0.0f, oy = 0.0f, sc = 1.0f, op = 1.0f;
        if (animation.enabled && !animation.animators.empty()) {
            for (const auto& animator : animation.animators) {
                const AnimatorSample sample = sample_animator(animator, layout, positioned, animation.time_seconds);
                ox += sample.offset_x;
                oy += sample.offset_y;
                sc *= sample.scale;
                op *= sample.opacity;
            }
        }

        renderer2d::Color color = style.fill_color;
        // Apply gradient if specified
        if (style.gradient.has_value()) {
            const auto& grad = *style.gradient;
            const float t = static_cast<float>(positioned.position.y) / static_cast<float>(std::max(1U, layout.height));
            if (grad.stops.size() >= 2) {
                const float clamped_t = std::clamp(t, 0.0f, 1.0f);
                for (size_t i = 0; i < grad.stops.size() - 1; ++i) {
                    if (clamped_t >= grad.stops[i].position && clamped_t <= grad.stops[i + 1].position) {
                        const float local_t = (clamped_t - grad.stops[i].position) / (grad.stops[i + 1].position - grad.stops[i].position);
                        color.r = grad.stops[i].color.r + (grad.stops[i + 1].color.r - grad.stops[i].color.r) * local_t;
                        color.g = grad.stops[i].color.g + (grad.stops[i + 1].color.g - grad.stops[i].color.g) * local_t;
                        color.b = grad.stops[i].color.b + (grad.stops[i + 1].color.b - grad.stops[i].color.b) * local_t;
                        break;
                    }
                }
            } else if (!grad.stops.empty()) {
                color = grad.stops[0].color;
            }
        }
        color.a *= op;
        const int dx = static_cast<int>(std::lround(positioned.position.x + ox));
        const int dy = static_cast<int>(std::lround(positioned.position.y + oy));
        const int dw = std::max(1, static_cast<int>(std::lround(static_cast<float>(glyph->width) * layout.scale * sc)));
        const int dh = std::max(1, static_cast<int>(std::lround(static_cast<float>(glyph->height) * layout.scale * sc)));
        surface.render_glyph(*glyph, dx, dy, dw, dh, color);
    }

    // Apply shadow and glow effects
    if (style.glow.enabled) {
        surface.apply_glow(style.glow);
    }
    if (style.shadow.enabled) {
        surface.apply_shadow(style.shadow);
    }

    return surface;
}

TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBoxSpec& text_box,
    const TextOutlineOptions& outline) {
    
    (void)outline;
    TextLayoutResult layout = layout_text(font, utf8_text, style, text_box);
    TextRasterSurface surface(layout.width, layout.height);
    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) return surface;

    for (const PositionedGlyph& positioned : layout.glyphs) {
        const GlyphBitmap* glyph = font.has_freetype_face() ? font.find_glyph_by_index(positioned.font_glyph_index) : font.find_scaled_glyph(positioned.codepoint, layout.scale);
        if (!glyph || glyph->width == 0U || glyph->height == 0U) continue;
        
        renderer2d::Color color = style.fill_color;
        // Apply gradient if specified
        if (style.gradient.has_value()) {
            const auto& grad = *style.gradient;
            const float t = static_cast<float>(positioned.position.y) / static_cast<float>(std::max(1U, layout.height));
            if (grad.stops.size() >= 2) {
                const float clamped_t = std::clamp(t, 0.0f, 1.0f);
                for (size_t i = 0; i < grad.stops.size() - 1; ++i) {
                    if (clamped_t >= grad.stops[i].position && clamped_t <= grad.stops[i + 1].position) {
                        const float local_t = (clamped_t - grad.stops[i].position) / (grad.stops[i + 1].position - grad.stops[i].position);
                        color.r = grad.stops[i].color.r + (grad.stops[i + 1].color.r - grad.stops[i].color.r) * local_t;
                        color.g = grad.stops[i].color.g + (grad.stops[i + 1].color.g - grad.stops[i].color.g) * local_t;
                        color.b = grad.stops[i].color.b + (grad.stops[i + 1].color.b - grad.stops[i].color.b) * local_t;
                        break;
                    }
                }
            } else if (!grad.stops.empty()) {
                color = grad.stops[0].color;
            }
        }
        
        const int dx = static_cast<int>(std::lround(positioned.position.x));
        const int dy = static_cast<int>(std::lround(positioned.position.y));
        const int dw = std::max(1, static_cast<int>(std::lround(static_cast<float>(glyph->width) * layout.scale)));
        const int dh = std::max(1, static_cast<int>(std::lround(static_cast<float>(glyph->height) * layout.scale)));
        surface.render_glyph(*glyph, dx, dy, dw, dh, color);
    }

    // Apply shadow and glow effects
    if (style.glow.enabled) {
        surface.apply_glow(style.glow);
    }
    if (style.shadow.enabled) {
        surface.apply_shadow(style.shadow);
    }

    return surface;
}

} // namespace tachyon::text
