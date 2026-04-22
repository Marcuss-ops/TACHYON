#pragma once

#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/text/layout/layout.h"
#include <optional>
#include <vector>
#include <span>

namespace tachyon::text {

struct TextAnimatorContext {
    std::size_t glyph_index{0};
    std::size_t cluster_index{0};
    std::size_t word_index{0};
    float total_glyphs{0.0f};
    float total_clusters{0.0f};
    float total_lines{0.0f};
    float time{0.0f};
};

template <typename T>
float sample_keyframe(const T& spec, const TextAnimatorContext& ctx) {
    (void)spec; (void)ctx;
    return 0.0f; // Placeholder implementation to restore build
}

double sample_scalar_kfs(
    const std::optional<double>& static_val,
    const std::vector<ScalarKeyframeSpec>& keyframes,
    float t);

math::Vector2 sample_vector2_kfs(
    const std::optional<math::Vector2>& static_val,
    const std::vector<Vector2KeyframeSpec>& keyframes,
    float t);

::tachyon::ColorSpec sample_color_kfs(
    const std::optional<::tachyon::ColorSpec>& static_val,
    const std::vector<ColorKeyframeSpec>& keyframes,
    float t);

float compute_coverage(const TextAnimatorSelectorSpec& selector, const TextAnimatorContext& ctx);

struct ResolvedGlyphPaint {
    const GlyphBitmap* glyph{nullptr};
    std::int32_t base_x{0};
    std::int32_t base_y{0};
    std::uint32_t target_width{0};
    std::uint32_t target_height{0};
    float opacity{1.0f};
    
    // New animatable fields
    ::tachyon::ColorSpec fill_color{255, 255, 255, 255};
    ::tachyon::ColorSpec stroke_color{0, 0, 0, 0};
    float stroke_width{0.0f};
    float tracking_offset{0.0f}; // Accumulated tracking
    
    std::size_t glyph_index{0};
};

std::vector<ResolvedGlyphPaint> resolve_glyph_paints(
    const BitmapFont& font,
    const TextLayoutResult& layout,
    const TextAnimationOptions& animation,
    std::span<const TextAnimatorSpec> animators = {});

} // namespace tachyon::text
