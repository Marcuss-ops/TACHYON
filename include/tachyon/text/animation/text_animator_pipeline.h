#pragma once

#include "tachyon/text/core/layout/resolved_text_layout.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/text/animation/text_animator_utils.h"
#include <span>
#include <cmath>
#include <algorithm>

namespace tachyon::text {

class TextAnimatorPipeline {
public:
    /**
     * @brief Applies a sequence of animators to a resolved layout mathematically.
     * Modifies the glyphs' position, scale, rotation, and styling in-place.
     * Preserves cluster boundaries for shape-preserving animation.
     * 
     * @param layout The base text layout to mutate.
     * @param animators The list of animator specifications.
     * @param context The evaluation context containing the current time.
     */
    static void apply_animators(
        ResolvedTextLayout& layout,
        std::span<const TextAnimatorSpec> animators,
        const TextAnimatorContext& context) {
        
        if (layout.glyphs.empty() || animators.empty()) return;

        const std::size_t num_glyphs = layout.glyphs.size();
        const float t = context.time;

        // Pre-compute total counts for selectors
        float total_clusters = 0.0f;
        float total_words = 0.0f;
        float total_lines = static_cast<float>(layout.lines.size());
        
        for (const auto& glyph : layout.glyphs) {
            total_clusters = std::max(total_clusters, static_cast<float>(glyph.cluster_index + 1));
            total_words = std::max(total_words, static_cast<float>(glyph.word_index + 1));
        }

        for (const auto& animator : animators) {
            
            // Sample the scalar/vector/color properties at time t
            double opacity = sample_scalar_kfs(animator.properties.opacity_value, animator.properties.opacity_keyframes, t);
            math::Vector2 pos_offset = sample_vector2_kfs(animator.properties.position_offset_value, animator.properties.position_offset_keyframes, t);
            double scale = sample_scalar_kfs(animator.properties.scale_value, animator.properties.scale_keyframes, t);
            double rotation = sample_scalar_kfs(animator.properties.rotation_value, animator.properties.rotation_keyframes, t);
            double tracking = sample_scalar_kfs(animator.properties.tracking_amount_value, animator.properties.tracking_amount_keyframes, t);
            ::tachyon::ColorSpec fill = sample_color_kfs(animator.properties.fill_color_value, animator.properties.fill_color_keyframes, t);
            ::tachyon::ColorSpec stroke = sample_color_kfs(animator.properties.stroke_color_value, animator.properties.stroke_color_keyframes, t);
            double stroke_width = sample_scalar_kfs(animator.properties.stroke_width_value, animator.properties.stroke_width_keyframes, t);
            double blur_radius = sample_scalar_kfs(animator.properties.blur_radius_value, animator.properties.blur_radius_keyframes, t);
            double reveal = sample_scalar_kfs(animator.properties.reveal_value, animator.properties.reveal_keyframes, t);

            float accumulated_tracking = 0.0f;

            for (std::size_t i = 0; i < num_glyphs; ++i) {
                auto& glyph = layout.glyphs[i];

                // 1. Evaluate Selector Coverage [0, 1] with cluster-aware context
                TextAnimatorContext ctx;
                ctx.glyph_index = i;
                ctx.cluster_index = glyph.cluster_index;
                ctx.word_index = static_cast<float>(glyph.word_index);
                
                // Find line index for this glyph
                ctx.line_index = 0;
                for (std::size_t l = 0; l < layout.lines.size(); ++l) {
                    if (i >= layout.lines[l].start_glyph_index && i < layout.lines[l].start_glyph_index + layout.lines[l].length) {
                        ctx.line_index = static_cast<float>(l);
                        break;
                    }
                }

                ctx.total_glyphs = static_cast<float>(num_glyphs);
                ctx.total_clusters = total_clusters;
                ctx.total_words = total_words;
                ctx.total_lines = total_lines;
                ctx.time = t;
                ctx.is_space = glyph.whitespace;
                ctx.is_rtl = glyph.is_rtl;

                float coverage = compute_coverage(animator.selector, ctx);
                
                // Accumulate tracking - applied to all glyphs to maintain layout flow
                accumulated_tracking += static_cast<float>(tracking) * coverage;
                glyph.position.x += accumulated_tracking;

                if (coverage <= 0.0f) continue;

                // 2. Apply animated properties scaled by coverage
                
                // Position Offset
                apply_position_offset(glyph, pos_offset, coverage);

                // Scale (preserve aspect ratio if only one component modified)
                float target_scale = static_cast<float>(scale);
                if (target_scale != 1.0f ||
                    has_scalar_property(animator.properties.scale_value, animator.properties.scale_keyframes)) {
                    apply_scale(glyph, target_scale, coverage);
                }

                // Rotation (in degrees, applied incrementally)
                apply_rotation(glyph, static_cast<float>(rotation), coverage);

                // Opacity
                if (has_scalar_property(animator.properties.opacity_value, animator.properties.opacity_keyframes)) {
                    apply_opacity(glyph, static_cast<float>(opacity), coverage);
                }

                // Fill Color
                if (has_color_property(animator.properties.fill_color_value, animator.properties.fill_color_keyframes)) {
                    apply_fill_color(glyph, fill, coverage);
                }

                // Stroke Color
                if (has_color_property(animator.properties.stroke_color_value, animator.properties.stroke_color_keyframes)) {
                    apply_stroke_color(glyph, stroke, coverage);
                }

                 // Stroke Width
                if (has_scalar_property(animator.properties.stroke_width_value, animator.properties.stroke_width_keyframes)) {
                    apply_stroke_width(glyph, static_cast<float>(stroke_width), coverage);
                }

                // Blur Radius
                if (has_scalar_property(animator.properties.blur_radius_value, animator.properties.blur_radius_keyframes)) {
                    apply_blur_radius(glyph, static_cast<float>(blur_radius), coverage);
                }

                // Reveal Factor (0.0 = hidden, 1.0 = fully revealed)
                if (has_scalar_property(animator.properties.reveal_value, animator.properties.reveal_keyframes)) {
                    apply_reveal_factor(glyph, static_cast<float>(reveal), coverage);
                }
            }
        }
    }

private:
    static bool has_scalar_property(const std::optional<double>& value,
                                    const std::vector<::tachyon::ScalarKeyframeSpec>& keyframes) {
        return value.has_value() || !keyframes.empty();
    }

    static bool has_color_property(const std::optional<::tachyon::ColorSpec>& value,
                                   const std::vector<::tachyon::ColorKeyframeSpec>& keyframes) {
        return value.has_value() || !keyframes.empty();
    }

    static void apply_position_offset(ResolvedGlyph& glyph,
                                      const math::Vector2& offset,
                                      float coverage) {
        glyph.position.x += offset.x * coverage;
        glyph.position.y += offset.y * coverage;
    }

    static void apply_scale(ResolvedGlyph& glyph,
                            float target_scale,
                            float coverage) {
        glyph.scale.x = glyph.scale.x * (1.0f - coverage) + (glyph.scale.x * target_scale) * coverage;
        glyph.scale.y = glyph.scale.y * (1.0f - coverage) + (glyph.scale.y * target_scale) * coverage;
    }

    static void apply_rotation(ResolvedGlyph& glyph,
                               float rotation,
                               float coverage) {
        glyph.rotation += rotation * coverage;
    }

    static void apply_opacity(ResolvedGlyph& glyph,
                              float opacity,
                              float coverage) {
        glyph.opacity = glyph.opacity * (1.0f - coverage) + opacity * coverage;
        glyph.opacity = std::clamp(glyph.opacity, 0.0f, 1.0f);
    }

    static void apply_fill_color(ResolvedGlyph& glyph,
                                 const ::tachyon::ColorSpec& color,
                                 float coverage) {
        glyph.fill_color = blend_color(glyph.fill_color, color, coverage);
    }

    static void apply_stroke_color(ResolvedGlyph& glyph,
                                   const ::tachyon::ColorSpec& color,
                                   float coverage) {
        glyph.stroke_color = blend_color(glyph.stroke_color, color, coverage);
    }

    static void apply_stroke_width(ResolvedGlyph& glyph,
                                   float stroke_width,
                                   float coverage) {
        glyph.stroke_width = glyph.stroke_width * (1.0f - coverage) + stroke_width * coverage;
    }

    static void apply_blur_radius(ResolvedGlyph& glyph,
                                  float blur_radius,
                                  float coverage) {
        glyph.blur_radius = glyph.blur_radius * (1.0f - coverage) + blur_radius * coverage;
        glyph.blur_radius = std::max(0.0f, glyph.blur_radius);
    }

    static void apply_reveal_factor(ResolvedGlyph& glyph,
                                    float reveal,
                                    float coverage) {
        glyph.reveal_factor = glyph.reveal_factor * (1.0f - coverage) + reveal * coverage;
        glyph.reveal_factor = std::clamp(glyph.reveal_factor, 0.0f, 1.0f);
    }

    static ::tachyon::ColorSpec blend_color(const ::tachyon::ColorSpec& a, const ::tachyon::ColorSpec& b, float t) {
        float clamped = std::clamp(t, 0.0f, 1.0f);
        auto lerp = [clamped](std::uint8_t x, std::uint8_t y) {
            return static_cast<std::uint8_t>(x + (y - x) * clamped);
        };
        return {lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a)};
    }
};

} // namespace tachyon::text
