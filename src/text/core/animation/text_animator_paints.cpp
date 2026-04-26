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

void apply_legacy_opacity_drop(TextLayoutResult& layout, float per_glyph_opacity_drop) {
    if (per_glyph_opacity_drop <= 0.0f) {
        return;
    }

    for (std::size_t i = 0; i < layout.glyphs.size(); ++i) {
        auto& glyph = layout.glyphs[i];
        const float dropped = glyph.opacity - per_glyph_opacity_drop * static_cast<float>(i);
        glyph.opacity = std::clamp(dropped, kZero, kOne);
    }
}

} // namespace

void apply_text_animators(
    TextLayoutResult& layout,
    std::span<const TextAnimatorSpec> animators,
    const TextAnimationOptions& animation) {

    if (!animation.enabled || layout.glyphs.empty()) {
        return;
    }

    if (animators.empty()) {
        apply_legacy_opacity_drop(layout, animation.per_glyph_opacity_drop);
        return;
    }

    const float t = animation.time_seconds;

    for (const auto& animator : animators) {
        float accumulated_tracking = 0.0f;

        auto get_stagger_index = [&](std::size_t glyph_idx) -> float {
            const auto& glyph = layout.glyphs[glyph_idx];
            if (animator.selector.stagger_mode == "character") {
                return static_cast<float>(glyph_idx);
            }
            if (animator.selector.stagger_mode == "word") {
                return static_cast<float>(glyph.word_index);
            }
            // line mode not supported - fallback to character index
            return static_cast<float>(glyph_idx);
        };

        for (std::size_t i = 0; i < layout.glyphs.size(); ++i) {
            auto& glyph = layout.glyphs[i];

            float staggered_t = t;
            if (animator.selector.stagger_mode != "none" && animator.selector.stagger_delay != 0.0) {
                const float stagger_offset = get_stagger_index(i) * static_cast<float>(animator.selector.stagger_delay);
                staggered_t = t - stagger_offset;
            }
            if (staggered_t < 0.0f) {
                staggered_t = 0.0f;
            }

            TextAnimatorContext ctx = make_text_animator_context(layout, i, staggered_t);
            ctx.is_space = ctx.is_space || glyph.whitespace;

            const float coverage = compute_coverage(animator.selector, ctx);
            if (coverage <= 0.0f) {
                continue;
            }

            const bool has_tracking = animator.properties.tracking_amount_value.has_value() || !animator.properties.tracking_amount_keyframes.empty();
            const bool has_position = animator.properties.position_offset_value.has_value() || !animator.properties.position_offset_keyframes.empty();
            const bool has_scale = animator.properties.scale_value.has_value() || !animator.properties.scale_keyframes.empty();
            const bool has_rotation = animator.properties.rotation_value.has_value() || !animator.properties.rotation_keyframes.empty();
            const bool has_opacity = animator.properties.opacity_value.has_value() || !animator.properties.opacity_keyframes.empty();
            const bool has_fill = animator.properties.fill_color_value.has_value() || !animator.properties.fill_color_keyframes.empty();
            const bool has_stroke = animator.properties.stroke_color_value.has_value() || !animator.properties.stroke_color_keyframes.empty();
            const bool has_stroke_width = animator.properties.stroke_width_value.has_value() || !animator.properties.stroke_width_keyframes.empty();
            const bool has_blur = animator.properties.blur_radius_value.has_value() || !animator.properties.blur_radius_keyframes.empty();
            const bool has_reveal = animator.properties.reveal_value.has_value() || !animator.properties.reveal_keyframes.empty();

            if (has_tracking) {
                const double tracking_value = sample_scalar_kfs(
                    animator.properties.tracking_amount_value,
                    animator.properties.tracking_amount_keyframes,
                    staggered_t);
                accumulated_tracking += static_cast<float>(tracking_value) * coverage;
                glyph.x += static_cast<std::int32_t>(std::lround(accumulated_tracking));
            }

            if (has_position) {
                const math::Vector2 pos_offset = sample_vector2_kfs(
                    animator.properties.position_offset_value,
                    animator.properties.position_offset_keyframes,
                    staggered_t);
                glyph.x += static_cast<std::int32_t>(std::lround(pos_offset.x * coverage));
                glyph.y += static_cast<std::int32_t>(std::lround(pos_offset.y * coverage));
            }

            if (has_scale) {
                const double scale_value = sample_scalar_kfs(
                    animator.properties.scale_value,
                    animator.properties.scale_keyframes,
                    staggered_t);
                const float target_scale = static_cast<float>(scale_value);
                glyph.scale.x = glyph.scale.x * (1.0f - coverage) + (glyph.scale.x * target_scale) * coverage;
                glyph.scale.y = glyph.scale.y * (1.0f - coverage) + (glyph.scale.y * target_scale) * coverage;
            }

            if (has_rotation) {
                const double rotation_value = sample_scalar_kfs(
                    animator.properties.rotation_value,
                    animator.properties.rotation_keyframes,
                    staggered_t);
                glyph.rotation += static_cast<float>(rotation_value) * coverage;
            }

            if (has_opacity) {
                const double opacity_value = sample_scalar_kfs(
                    animator.properties.opacity_value,
                    animator.properties.opacity_keyframes,
                    staggered_t);
                const float target_opacity = static_cast<float>(opacity_value);
                glyph.opacity = glyph.opacity * (1.0f - coverage) + target_opacity * coverage;
                glyph.opacity = std::clamp(glyph.opacity, kZero, kOne);
            }

            if (has_fill) {
                const ::tachyon::ColorSpec sampled = sample_color_kfs(
                    animator.properties.fill_color_value,
                    animator.properties.fill_color_keyframes,
                    staggered_t);
                glyph.fill_color = blend_color(glyph.fill_color, sampled, coverage);
            }

            if (has_stroke) {
                const ::tachyon::ColorSpec sampled = sample_color_kfs(
                    animator.properties.stroke_color_value,
                    animator.properties.stroke_color_keyframes,
                    staggered_t);
                glyph.stroke_color = blend_color(glyph.stroke_color, sampled, coverage);
            }

            if (has_stroke_width) {
                const double stroke_width_value = sample_scalar_kfs(
                    animator.properties.stroke_width_value,
                    animator.properties.stroke_width_keyframes,
                    staggered_t);
                glyph.stroke_width = glyph.stroke_width * (1.0f - coverage) + static_cast<float>(stroke_width_value) * coverage;
            }

            if (has_blur) {
                const double blur_value = sample_scalar_kfs(
                    animator.properties.blur_radius_value,
                    animator.properties.blur_radius_keyframes,
                    staggered_t);
                glyph.blur_radius = std::max(0.0f, glyph.blur_radius * (1.0f - coverage) + static_cast<float>(blur_value) * coverage);
            }

            if (has_reveal) {
                const double reveal_value = sample_scalar_kfs(
                    animator.properties.reveal_value,
                    animator.properties.reveal_keyframes,
                    staggered_t);
                glyph.reveal_factor = std::clamp(
                    glyph.reveal_factor * (1.0f - coverage) + static_cast<float>(reveal_value) * coverage,
                    kZero,
                    kOne);
            }
        }
    }

    apply_legacy_opacity_drop(layout, animation.per_glyph_opacity_drop);
}

std::vector<ResolvedGlyphPaint> resolve_glyph_paints(
    const BitmapFont& font,
    const TextLayoutResult& layout,
    const TextAnimationOptions& animation) {

    TextLayoutResult animated_layout = layout;
    apply_text_animators(animated_layout, animation.animators, animation);

    std::vector<ResolvedGlyphPaint> paints;
    paints.reserve(animated_layout.glyphs.size());

    for (std::size_t idx = 0; idx < animated_layout.glyphs.size(); ++idx) {
        const PositionedGlyph& positioned = animated_layout.glyphs[idx];
        const GlyphBitmap* glyph = font.has_freetype_face()
            ? font.find_glyph_by_index(positioned.font_glyph_index)
            : font.find_scaled_glyph(positioned.codepoint, layout.scale);
        if (glyph == nullptr || glyph->width == 0U || glyph->height == 0U) {
            continue;
        }

        ResolvedGlyphPaint paint;
        paint.glyph = glyph;
        paint.base_x = positioned.x;
        paint.base_y = positioned.y;
        paint.target_width = static_cast<std::uint32_t>(std::max(1, static_cast<std::int32_t>(std::lround(static_cast<float>(glyph->width) * positioned.scale.x))));
        paint.target_height = static_cast<std::uint32_t>(std::max(1, static_cast<std::int32_t>(std::lround(static_cast<float>(glyph->height) * positioned.scale.y))));
        paint.opacity = std::clamp(positioned.opacity * positioned.reveal_factor, kZero, kOne);
        paint.fill_color = positioned.fill_color;
        paint.stroke_color = positioned.stroke_color;
        paint.stroke_width = positioned.stroke_width;
        paint.tracking_offset = static_cast<float>(positioned.x - layout.glyphs[idx].x);
        paint.glyph_index = positioned.glyph_index;
        paints.push_back(paint);
    }

    return paints;
}

} // namespace tachyon::text
