#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/text/fonts/font.h"
#include <algorithm>
#include <cmath>

namespace tachyon::text {

namespace {

constexpr float kZero = 0.0f;
constexpr float kOne = 1.0f;

template <typename T>
T lerp_channel(T a, T b, float t) {
    return static_cast<T>(a + (b - a) * t);
}

::tachyon::ColorSpec blend_color(const ::tachyon::ColorSpec& a, const ::tachyon::ColorSpec& b, float t) {
    const float clamped = std::clamp(t, kZero, kOne);
    return {
        lerp_channel<std::uint8_t>(a.r, b.r, clamped),
        lerp_channel<std::uint8_t>(a.g, b.g, clamped),
        lerp_channel<std::uint8_t>(a.b, b.b, clamped),
        lerp_channel<std::uint8_t>(a.a, b.a, clamped)
    };
}

} // namespace

std::vector<ResolvedGlyphPaint> resolve_glyph_paints(
    const BitmapFont& font,
    const TextLayoutResult& layout,
    const TextAnimationOptions& animation) {

    std::vector<ResolvedGlyphPaint> paints;
    paints.reserve(layout.glyphs.size());

    for (std::size_t idx = 0; idx < layout.glyphs.size(); ++idx) {
        const PositionedGlyph& positioned = layout.glyphs[idx];
        const GlyphBitmap* glyph = font.has_freetype_face()
            ? font.find_glyph_by_index(positioned.font_glyph_index)
            : font.find_scaled_glyph(positioned.codepoint, layout.scale);
        if (glyph == nullptr || glyph->width == 0U || glyph->height == 0U) {
            continue;
        }

        float glyph_offset_x = kZero;
        float glyph_offset_y = kZero;
        float glyph_scale = kOne;
        float glyph_opacity = kOne;
        float tracking_offset = kZero;
        ::tachyon::ColorSpec fill_color = {255, 255, 255, 255};
        ::tachyon::ColorSpec stroke_color = {0, 0, 0, 0};
        float stroke_width = kZero;

        for (const auto& animator : animation.animators) {
            TextAnimatorContext ctx = make_text_animator_context(layout, idx, animation.time_seconds);

            const float coverage = compute_coverage(animator.selector, ctx);
            if (coverage <= 0.0f) {
                continue;
            }

            if (animator.properties.tracking_amount_value.has_value() || !animator.properties.tracking_amount_keyframes.empty()) {
                const double tracking_value = sample_scalar_kfs(animator.properties.tracking_amount_value, animator.properties.tracking_amount_keyframes, animation.time_seconds);
                tracking_offset += static_cast<float>(tracking_value) * coverage;
            }

            if (animator.properties.fill_color_value.has_value() || !animator.properties.fill_color_keyframes.empty()) {
                const ::tachyon::ColorSpec sampled = sample_color_kfs(animator.properties.fill_color_value, animator.properties.fill_color_keyframes, animation.time_seconds);
                fill_color = blend_color(fill_color, sampled, coverage);
            }

            if (animator.properties.stroke_color_value.has_value() || !animator.properties.stroke_color_keyframes.empty()) {
                const ::tachyon::ColorSpec sampled = sample_color_kfs(animator.properties.stroke_color_value, animator.properties.stroke_color_keyframes, animation.time_seconds);
                stroke_color = blend_color(stroke_color, sampled, coverage);
            }

            if (animator.properties.stroke_width_value.has_value() || !animator.properties.stroke_width_keyframes.empty()) {
                const double sampled = sample_scalar_kfs(animator.properties.stroke_width_value, animator.properties.stroke_width_keyframes, animation.time_seconds);
                stroke_width += static_cast<float>(sampled) * coverage;
            }
        }

        ResolvedGlyphPaint paint;
        paint.glyph = glyph;
        paint.base_x = positioned.x + static_cast<std::int32_t>(std::lround(glyph_offset_x + tracking_offset));
        paint.base_y = positioned.y + static_cast<std::int32_t>(std::lround(glyph_offset_y));
        paint.target_width = static_cast<std::uint32_t>(std::max(1, static_cast<std::int32_t>(std::lround(static_cast<float>(glyph->width) * glyph_scale))));
        paint.target_height = static_cast<std::uint32_t>(std::max(1, static_cast<std::int32_t>(std::lround(static_cast<float>(glyph->height) * glyph_scale))));
        paint.opacity = std::clamp(glyph_opacity, kZero, kOne);
        paint.fill_color = fill_color;
        paint.stroke_color = stroke_color;
        paint.stroke_width = stroke_width;
        paint.tracking_offset = tracking_offset;
        paint.glyph_index = positioned.glyph_index;
        paints.push_back(paint);
    }

    return paints;
}

} // namespace tachyon::text
