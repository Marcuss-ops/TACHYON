#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/core/animation/easing.h"
#include <algorithm>
#include <cmath>

namespace tachyon::text {

namespace {

template <typename T>
T lerp_channel(T a, T b, float t) {
    return static_cast<T>(a + (b - a) * t);
}

::tachyon::ColorSpec blend_color(const ::tachyon::ColorSpec& a, const ::tachyon::ColorSpec& b, float t) {
    const float clamped = std::clamp(t, 0.0f, 1.0f);
    return {
        lerp_channel<std::uint8_t>(a.r, b.r, clamped),
        lerp_channel<std::uint8_t>(a.g, b.g, clamped),
        lerp_channel<std::uint8_t>(a.b, b.b, clamped),
        lerp_channel<std::uint8_t>(a.a, b.a, clamped)
    };
}

} // namespace

double sample_scalar_kfs(
    const std::optional<double>& static_val,
    const std::vector<ScalarKeyframeSpec>& keyframes,
    float t) {
    if (keyframes.empty()) return static_val.value_or(0.0);
    if (keyframes.size() == 1U || t <= static_cast<float>(keyframes.front().time)) return keyframes.front().value;
    if (t >= static_cast<float>(keyframes.back().time)) return keyframes.back().value;
    for (std::size_t i = 0; i + 1 < keyframes.size(); ++i) {
        const double t0 = keyframes[i].time, t1 = keyframes[i+1].time;
        if (static_cast<double>(t) >= t0 && static_cast<double>(t) <= t1) {
            const double raw = (t1 > t0) ? (static_cast<double>(t) - t0) / (t1 - t0) : 0.0;
            const double eased = tachyon::animation::apply_easing(raw, keyframes[i].easing, keyframes[i].bezier);
            return keyframes[i].value + eased * (keyframes[i+1].value - keyframes[i].value);
        }
    }
    return keyframes.back().value;
}

::tachyon::ColorSpec sample_color_kfs(
    const std::optional<::tachyon::ColorSpec>& static_val,
    const std::vector<ColorKeyframeSpec>& keyframes,
    float t) {
    if (keyframes.empty()) {
        return static_val.value_or(::tachyon::ColorSpec{255, 255, 255, 255});
    }
    if (keyframes.size() == 1U || t <= static_cast<float>(keyframes.front().time)) {
        return keyframes.front().value;
    }
    if (t >= static_cast<float>(keyframes.back().time)) {
        return keyframes.back().value;
    }

    for (std::size_t i = 0; i + 1 < keyframes.size(); ++i) {
        const double t0 = keyframes[i].time;
        const double t1 = keyframes[i + 1].time;
        if (static_cast<double>(t) >= t0 && static_cast<double>(t) <= t1) {
            const double raw = (t1 > t0) ? (static_cast<double>(t) - t0) / (t1 - t0) : 0.0;
            const float eased = static_cast<float>(tachyon::animation::apply_easing(raw, keyframes[i].easing, keyframes[i].bezier));
            return blend_color(keyframes[i].value, keyframes[i + 1].value, eased);
        }
    }

    return keyframes.back().value;
}

math::Vector2 sample_vector2_kfs(
    const std::optional<math::Vector2>& static_val,
    const std::vector<Vector2KeyframeSpec>& keyframes,
    float t) {
    if (keyframes.empty()) return static_val.value_or(math::Vector2{0.0f, 0.0f});
    if (keyframes.size() == 1U || t <= static_cast<float>(keyframes.front().time)) return keyframes.front().value;
    if (t >= static_cast<float>(keyframes.back().time)) return keyframes.back().value;
    for (std::size_t i = 0; i + 1 < keyframes.size(); ++i) {
        const double t0 = keyframes[i].time, t1 = keyframes[i+1].time;
        if (static_cast<double>(t) >= t0 && static_cast<double>(t) <= t1) {
            const double raw = (t1 > t0) ? (static_cast<double>(t) - t0) / (t1 - t0) : 0.0;
            const float a = static_cast<float>(tachyon::animation::apply_easing(raw, keyframes[i].easing, keyframes[i].bezier));
            const math::Vector2& va = keyframes[i].value;
            const math::Vector2& vb = keyframes[i+1].value;
            return math::Vector2{va.x + (vb.x - va.x) * a, va.y + (vb.y - va.y) * a};
        }
    }
    return keyframes.back().value;
}

float compute_coverage(const TextAnimatorSelectorSpec& selector, const TextAnimatorContext& ctx) {
    if (selector.type == "all" || ctx.total_glyphs == 0.0f) return 1.0f;
    
    // Select the basis for the selector (characters, words, etc.)
    float t = 0.0f;
    if (selector.based_on == "words" && ctx.total_clusters > 0) {
        // This is a simplification; a real implementation would use ctx.word_index
        t = static_cast<float>(ctx.glyph_index) / ctx.total_glyphs;
    } else {
        t = static_cast<float>(ctx.glyph_index) / ctx.total_glyphs;
    }

    const float start = static_cast<float>(selector.start) / 100.0f; // AE selectors are usually percent
    const float end   = static_cast<float>(selector.end) / 100.0f;
    const float span  = end - start;
    if (std::abs(span) < 1e-6f) return (t >= start) ? 1.0f : 0.0f;
    return std::clamp((t - start) / span, 0.0f, 1.0f);
}

std::vector<ResolvedGlyphPaint> resolve_glyph_paints(
    const BitmapFont& font,
    const TextLayoutResult& layout,
    const TextAnimationOptions& animation,
    std::span<const TextAnimatorSpec> animators) {

    std::vector<ResolvedGlyphPaint> paints;
    paints.reserve(layout.glyphs.size());

    for (const PositionedGlyph& positioned : layout.glyphs) {
        const GlyphBitmap* glyph = font.has_freetype_face()
            ? font.find_glyph_by_index(positioned.font_glyph_index)
            : font.find_scaled_glyph(positioned.codepoint, layout.scale);
        if (glyph == nullptr || glyph->width == 0U || glyph->height == 0U) {
            continue;
        }

        float glyph_offset_x = 0.0f;
        float glyph_offset_y = 0.0f;
        float glyph_scale = 1.0f;
        float glyph_opacity = 1.0f;
        float tracking_offset = 0.0f;
        ::tachyon::ColorSpec fill_color = {255, 255, 255, 255};
        ::tachyon::ColorSpec stroke_color = {0, 0, 0, 0};
        float stroke_width = 0.0f;
        if (animation.enabled) {
            const float phase_seconds = animation.time_seconds - static_cast<float>(positioned.glyph_index) * 0.1f;
            const float wave_period = std::max(0.001f, animation.wave_period_seconds);
            const float wave_phase = (phase_seconds / wave_period) * 6.28318530717958647692f;

            glyph_offset_x = animation.per_glyph_offset_x * static_cast<float>(positioned.glyph_index) + std::sin(wave_phase) * animation.wave_amplitude_x;
            glyph_offset_y = animation.per_glyph_offset_y * static_cast<float>(positioned.glyph_index) + std::cos(wave_phase) * animation.wave_amplitude_y;
            glyph_scale = std::max(0.05f, 1.0f + animation.per_glyph_scale_delta * static_cast<float>(positioned.glyph_index));
            glyph_opacity = std::clamp(1.0f - animation.per_glyph_opacity_drop * static_cast<float>(positioned.glyph_index), 0.0f, 1.0f);
        }

        for (const auto& animator : animators) {
            TextAnimatorContext ctx;
            ctx.glyph_index = positioned.glyph_index;
            ctx.cluster_index = positioned.cluster_index;
            ctx.total_glyphs = static_cast<float>(layout.glyphs.size());
            ctx.time = animation.time_seconds;
            
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
        paint.opacity = std::clamp(glyph_opacity, 0.0f, 1.0f);
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
