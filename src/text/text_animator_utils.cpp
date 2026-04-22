#include "tachyon/text/text_animator_utils.h"
#include <algorithm>
#include <cmath>

namespace tachyon::text {

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
            const double a = (t1 > t0) ? (static_cast<double>(t) - t0) / (t1 - t0) : 0.0;
            return keyframes[i].value + a * (keyframes[i+1].value - keyframes[i].value);
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
            const float a = static_cast<float>((t1 > t0) ? (static_cast<double>(t) - t0) / (t1 - t0) : 0.0);
            const math::Vector2& va = keyframes[i].value;
            const math::Vector2& vb = keyframes[i+1].value;
            return math::Vector2{va.x + (vb.x - va.x) * a, va.y + (vb.y - va.y) * a};
        }
    }
    return keyframes.back().value;
}

float compute_coverage(const TextAnimatorSelectorSpec& selector, std::size_t i, std::size_t N) {
    if (selector.type == "all" || N == 0U) return 1.0f;
    const float t = static_cast<float>(i) / static_cast<float>(N);
    const float start = static_cast<float>(selector.start);
    const float end   = static_cast<float>(selector.end);
    const float span  = end - start;
    if (span <= 0.0f) return (t >= start) ? 1.0f : 0.0f;
    return std::clamp((t - start) / span, 0.0f, 1.0f);
}

std::vector<ResolvedGlyphPaint> resolve_glyph_paints(
    const BitmapFont& font,
    const TextLayoutResult& layout,
    const TextAnimationOptions& animation) {

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
        if (animation.enabled) {
            const float phase_seconds = animation.time_seconds - static_cast<float>(positioned.glyph_index) * 0.1f;
            const float wave_period = std::max(0.001f, animation.wave_period_seconds);
            const float wave_phase = (phase_seconds / wave_period) * 6.28318530717958647692f;

            glyph_offset_x = animation.per_glyph_offset_x * static_cast<float>(positioned.glyph_index) + std::sin(wave_phase) * animation.wave_amplitude_x;
            glyph_offset_y = animation.per_glyph_offset_y * static_cast<float>(positioned.glyph_index) + std::cos(wave_phase) * animation.wave_amplitude_y;
            glyph_scale = std::max(0.05f, 1.0f + animation.per_glyph_scale_delta * static_cast<float>(positioned.glyph_index));
            glyph_opacity = std::clamp(1.0f - animation.per_glyph_opacity_drop * static_cast<float>(positioned.glyph_index), 0.0f, 1.0f);
        }

        ResolvedGlyphPaint paint;
        paint.glyph = glyph;
        paint.base_x = positioned.x + static_cast<std::int32_t>(std::lround(glyph_offset_x));
        paint.base_y = positioned.y + static_cast<std::int32_t>(std::lround(glyph_offset_y));
        paint.target_width = static_cast<std::uint32_t>(std::max(1, static_cast<std::int32_t>(std::lround(static_cast<float>(glyph->width) * glyph_scale))));
        paint.target_height = static_cast<std::uint32_t>(std::max(1, static_cast<std::int32_t>(std::lround(static_cast<float>(glyph->height) * glyph_scale))));
        paint.opacity = std::clamp(glyph_opacity, 0.0f, 1.0f);
        paint.glyph_index = positioned.glyph_index;
        paints.push_back(paint);
    }

    return paints;
}

} // namespace tachyon::text
