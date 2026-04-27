#pragma once

#include "tachyon/text/animation/text_animation_options.h"
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
    std::size_t line_index{0};
    float total_glyphs{0.0f};
    float total_clusters{0.0f};
    float total_words{0.0f};
    float total_lines{0.0f};
    float non_space_glyph_index{0.0f};
    float total_non_space_glyphs{0.0f};
    float time{0.0f};
    
    // Cluster-aware fields for shape-preserving animation
    std::size_t cluster_codepoint_start{0};
    std::size_t cluster_codepoint_count{1};
    bool is_space{false};  // True if this glyph represents whitespace
    bool is_rtl{false};   // True if this glyph is in RTL run
};

// Keyframe sampling — canonical entry points backed by AnimationCurve<T>.
// See text_animator_sampling.cpp for the full Bezier/spring/easing implementation.

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

float evaluate_expression_wrapper(const std::string& expr, const TextAnimatorContext& ctx);

void apply_text_animators(
    TextLayoutResult& layout,
    std::span<const TextAnimatorSpec> animators,
    const TextAnimationOptions& animation);

TextAnimatorContext make_text_animator_context(
    const ResolvedTextLayout& layout,
    std::size_t glyph_index,
    float time);

TextAnimatorContext make_text_animator_context(
    const TextLayoutResult& layout,
    std::size_t glyph_index,
    float time);

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
    const TextAnimationOptions& animation);

} // namespace tachyon::text
