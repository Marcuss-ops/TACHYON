#include "tachyon/text/layout/layout.h"
#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/text/animation/text_on_path.h"

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
    const std::vector<ResolvedGlyphPaint> paints = resolve_glyph_paints(font, layout, animation);

    for (const auto& paint : paints) {
        if (paint.glyph == nullptr) {
            continue;
        }
        surface.render_glyph(
            *paint.glyph,
            paint.base_x,
            paint.base_y,
            static_cast<int>(paint.target_width),
            static_cast<int>(paint.target_height),
            renderer2d::Color{
                static_cast<float>(paint.fill_color.r) / 255.0f,
                static_cast<float>(paint.fill_color.g) / 255.0f,
                static_cast<float>(paint.fill_color.b) / 255.0f,
                static_cast<float>(paint.fill_color.a) / 255.0f * paint.opacity
            });
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
    const TextBox& text_box,
    TextAlignment alignment,
    const TextAnimationOptions& animation,
    const TextLayoutOptions& layout_options) {

    const TextLayoutResult layout = layout_text(font, utf8_text, style, text_box, alignment, layout_options);
    TextRasterSurface surface(layout.width, layout.height);
    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) {
        return surface;
    }

    const std::vector<ResolvedGlyphPaint> paints = resolve_glyph_paints(font, layout, animation);

    for (const auto& paint : paints) {
        if (paint.glyph == nullptr) {
            continue;
        }
        surface.render_glyph_with_motion_blur(
            *paint.glyph,
            paint.base_x,
            paint.base_y,
            static_cast<int>(paint.target_width),
            static_cast<int>(paint.target_height),
            renderer2d::Color{
                static_cast<float>(paint.fill_color.r) / 255.0f,
                static_cast<float>(paint.fill_color.g) / 255.0f,
                static_cast<float>(paint.fill_color.b) / 255.0f,
                static_cast<float>(paint.fill_color.a) / 255.0f * paint.opacity
            },
            paint.motion_blur_vector.x,
            paint.motion_blur_vector.y);
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
    const TextBox& text_box,
    TextAlignment alignment,
    std::span<const TextHighlightSpan> highlights,
    const TextLayoutOptions& layout_options,
    const TextAnimationOptions& animation) {

    const TextLayoutResult layout = layout_text(font, utf8_text, style, text_box, alignment, layout_options);
    TextRasterSurface surface(layout.width, layout.height);
    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) return surface;
    (void)animation;

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

    const std::vector<ResolvedGlyphPaint> paints = resolve_glyph_paints(font, layout, animation);
    for (const auto& paint : paints) {
        if (paint.glyph == nullptr) {
            continue;
        }
        surface.render_glyph(
            *paint.glyph,
            paint.base_x,
            paint.base_y,
            static_cast<int>(paint.target_width),
            static_cast<int>(paint.target_height),
            renderer2d::Color{
                static_cast<float>(paint.fill_color.r) / 255.0f,
                static_cast<float>(paint.fill_color.g) / 255.0f,
                static_cast<float>(paint.fill_color.b) / 255.0f,
                static_cast<float>(paint.fill_color.a) / 255.0f * paint.opacity
            });
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
    const TextBox& text_box,
    TextAlignment alignment,
    const TextOutlineOptions& outline) {
    
    (void)outline;
    TextLayoutOptions layout_options; // Default options
    TextLayoutResult layout = layout_text(font, utf8_text, style, text_box, alignment, layout_options);
    TextRasterSurface surface(layout.width, layout.height);
    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) return surface;

    for (const PositionedGlyph& positioned : layout.glyphs) {
        const GlyphBitmap* glyph = nullptr;
        if (font.has_freetype_face()) {
            glyph = layout_options.use_sdf ? font.find_sdf_glyph_by_index(positioned.font_glyph_index) : font.find_glyph_by_index(positioned.font_glyph_index);
        } else {
            glyph = font.find_scaled_glyph(positioned.codepoint, layout.scale);
        }
        
        if (!glyph || glyph->width == 0U || glyph->height == 0U) continue;
        
        renderer2d::Color color = style.fill_color;
        // Apply gradient if specified
        if (style.gradient.has_value()) {
            const auto& grad = *style.gradient;
            const float t = static_cast<float>(positioned.y) / static_cast<float>(std::max(1U, layout.height));
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
        
        surface.render_glyph(*glyph, positioned.x, positioned.y, (int)std::lround((float)glyph->width * layout.scale), (int)std::lround((float)glyph->height * layout.scale), color);
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
    const TextLayoutResult& layout,
    const TextAnimationOptions& animation) {

    TextRasterSurface surface(layout.width, layout.height);
    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) return surface;

    const std::vector<ResolvedGlyphPaint> paints = resolve_glyph_paints(font, layout, animation);

    for (const auto& paint : paints) {
        if (paint.glyph == nullptr) continue;
        surface.render_glyph(
            *paint.glyph,
            paint.base_x,
            paint.base_y,
            static_cast<int>(paint.target_width),
            static_cast<int>(paint.target_height),
            renderer2d::Color{
                static_cast<float>(paint.fill_color.r) / 255.0f,
                static_cast<float>(paint.fill_color.g) / 255.0f,
                static_cast<float>(paint.fill_color.b) / 255.0f,
                static_cast<float>(paint.fill_color.a) / 255.0f * paint.opacity
            });
    }

    return surface;
}

TextRasterSurface rasterize_text_rgba(
    const BitmapFont& font,
    const TextLayoutResult& layout,
    const std::vector<ResolvedGlyphPaint>& paints) {

    TextRasterSurface surface(layout.width, layout.height);
    if (!font.is_loaded() || layout.width == 0U || layout.height == 0U) return surface;

    for (const auto& paint : paints) {
        if (paint.glyph == nullptr) continue;
        surface.render_glyph(
            *paint.glyph,
            paint.base_x,
            paint.base_y,
            static_cast<int>(paint.target_width),
            static_cast<int>(paint.target_height),
            renderer2d::Color{
                static_cast<float>(paint.fill_color.r) / 255.0f,
                static_cast<float>(paint.fill_color.g) / 255.0f,
                static_cast<float>(paint.fill_color.b) / 255.0f,
                static_cast<float>(paint.fill_color.a) / 255.0f * paint.opacity
            });
    }

    return surface;
}

ResolvedTextLayout layout_text_on_path(
    const BitmapFont& font,
    std::string_view utf8_text,
    const TextStyle& style,
    const TextBox& text_box,
    TextAlignment alignment,
    const shapes::ShapePath& path,
    double path_offset,
    bool align_perpendicular,
    const TextLayoutOptions& options) {

    // 1. Standard layout pass
    TextLayoutResult result = layout_text(font, utf8_text, style, text_box, alignment, options);

    // 2. Map glyphs onto path using arc-length parameterization
    TextOnPathModifier::apply(
        static_cast<ResolvedTextLayout&>(result),
        path,
        path_offset,
        align_perpendicular);

    return static_cast<ResolvedTextLayout>(result);
}

} // namespace tachyon::text
