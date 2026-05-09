#pragma once

#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/text/layout/layout.h"
#include <optional>
#include <string_view>
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
    float total_non_space_glyphs{0.0f};
    float non_space_glyph_index{0.0f};
    float time{0.0f};
    
    // Cluster-aware fields for shape-preserving animation
    std::size_t cluster_codepoint_start{0};
    std::size_t cluster_codepoint_count{1};
    bool is_space{false};  // True if this glyph represents whitespace
    bool is_rtl{false};   // True if this glyph is in RTL run
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

::tachyon::math::Vector2 sample_vector2_kfs(
    const std::optional<::tachyon::math::Vector2>& static_val,
    const std::vector<Vector2KeyframeSpec>& keyframes,
    float t);

::tachyon::ColorSpec sample_color_kfs(
    const std::optional<::tachyon::ColorSpec>& static_val,
    const std::vector<ColorKeyframeSpec>& keyframes,
    float t);

float compute_coverage(const TextAnimatorSelectorSpec& selector, const TextAnimatorContext& ctx);
float evaluate_expression_wrapper(const std::string& expr, const TextAnimatorContext& ctx);

inline bool uses_character_stagger_layout(const TextAnimatorSpec& animator) {
    return animator.selector.stagger_mode == "character" &&
           (animator.selector.based_on == "characters" ||
            animator.selector.based_on == "characters_excluding_spaces" ||
            animator.selector.based_on == "clusters");
}

inline bool prefers_fixed_pitch_layout(std::span<const TextAnimatorSpec> animators) {
    for (const auto& animator : animators) {
        if (uses_character_stagger_layout(animator)) {
            return true;
        }
    }
    return false;
}

struct ResolvedGlyphPaint {
    const GlyphBitmap* glyph{nullptr};
    float base_x{0.0f};
    float base_y{0.0f};
    float target_width{0.0f};
    float target_height{0.0f};
    float opacity{1.0f};
    ::tachyon::math::Vector2 motion_blur_vector{0.0f, 0.0f};
    
    // New animatable fields
    ::tachyon::ColorSpec fill_color{255, 255, 255, 255};
    ::tachyon::ColorSpec stroke_color{0, 0, 0, 0};
    float stroke_width{0.0f};
    float tracking_offset{0.0f}; // Accumulated tracking
    bool is_cursor{false};
    
    std::size_t glyph_index{0};
};

void apply_text_animators(
    TextLayoutResult& layout,
    const TextAnimationOptions& animation);

std::vector<ResolvedGlyphPaint> resolve_glyph_paints(
    const BitmapFont& font,
    const TextLayoutResult& layout,
    const TextAnimationOptions& animation,
    std::span<const TextAnimatorSpec> animators = {});

} // namespace tachyon::text
