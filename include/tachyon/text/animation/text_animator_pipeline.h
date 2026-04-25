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

        for (const auto& animator : animators) {
            
            // Calculate stagger offset multiplier based on mode
            auto get_stagger_index = [&](std::size_t glyph_idx) -> float {
                const auto& glyph = layout.glyphs[glyph_idx];
                if (animator.selector.stagger_mode == "character") {
                    return static_cast<float>(glyph_idx);
                } else if (animator.selector.stagger_mode == "word") {
                    return static_cast<float>(glyph.word_index);
                } else if (animator.selector.stagger_mode == "line") {
                    return static_cast<float>(glyph.line_index);
                }
                return 0.0f;
            };

            float accumulated_tracking = 0.0f;

            for (std::size_t i = 0; i < num_glyphs; ++i) {
                auto& glyph = layout.glyphs[i];

                // Apply stagger delay to time for this glyph
                float staggered_t = t;
                if (animator.selector.stagger_mode != "none" && animator.selector.stagger_delay != 0.0) {
                    float stagger_idx = get_stagger_index(i);
                    float stagger_offset = stagger_idx * static_cast<float>(animator.selector.stagger_delay);
                    staggered_t = t - stagger_offset;
                }
                // Ensure we don't go negative if properties don't handle it
                if (staggered_t < 0.0f) staggered_t = 0.0f;

                // Sample properties at staggered time for this glyph
                double opacity = sample_scalar_kfs(animator.properties.opacity_value, animator.properties.opacity_keyframes, staggered_t);
                math::Vector2 pos_offset = sample_vector2_kfs(animator.properties.position_offset_value, animator.properties.position_offset_keyframes, staggered_t);
                double scale = sample_scalar_kfs(animator.properties.scale_value, animator.properties.scale_keyframes, staggered_t);
                double rotation = sample_scalar_kfs(animator.properties.rotation_value, animator.properties.rotation_keyframes, staggered_t);
                double tracking = sample_scalar_kfs(animator.properties.tracking_amount_value, animator.properties.tracking_amount_keyframes, staggered_t);
                ::tachyon::ColorSpec fill = sample_color_kfs(animator.properties.fill_color_value, animator.properties.fill_color_keyframes, staggered_t);
                ::tachyon::ColorSpec stroke = sample_color_kfs(animator.properties.stroke_color_value, animator.properties.stroke_color_keyframes, staggered_t);
                double stroke_width = sample_scalar_kfs(animator.properties.stroke_width_value, animator.properties.stroke_width_keyframes, staggered_t);
                double blur_radius = sample_scalar_kfs(animator.properties.blur_radius_value, animator.properties.blur_radius_keyframes, staggered_t);
                double reveal = sample_scalar_kfs(animator.properties.reveal_value, animator.properties.reveal_keyframes, staggered_t);

                // 1. Evaluate selector coverage using the shared context builder.
                TextAnimatorContext ctx = make_text_animator_context(layout, i, staggered_t);
                ctx.is_space = ctx.is_space || (glyph.bounds.width <= 0.0f && glyph.bounds.height <= 0.0f);

                float coverage = compute_coverage(animator.selector, ctx);
                
                // Accumulate tracking - applied to all glyphs to maintain layout flow
                accumulated_tracking += static_cast<float>(tracking) * coverage;
                glyph.position.x += accumulated_tracking;

                if (coverage <= 0.0f) continue;

                // 2. Apply animated properties scaled by coverage
                
                // Position Offset
                glyph.position.x += pos_offset.x * coverage;
                glyph.position.y += pos_offset.y * coverage;

                // Scale (preserve aspect ratio if only one component modified)
                float target_scale = static_cast<float>(scale);
                if (target_scale != 1.0f || animator.properties.scale_value.has_value() || !animator.properties.scale_keyframes.empty()) {
                    glyph.scale.x = glyph.scale.x * (1.0f - coverage) + (glyph.scale.x * target_scale) * coverage;
                    glyph.scale.y = glyph.scale.y * (1.0f - coverage) + (glyph.scale.y * target_scale) * coverage;
                }

                // Rotation (in degrees, applied incrementally)
                glyph.rotation += static_cast<float>(rotation) * coverage;

                // Opacity
                if (animator.properties.opacity_value.has_value() || !animator.properties.opacity_keyframes.empty()) {
                    float target_op = static_cast<float>(opacity);
                    glyph.opacity = glyph.opacity * (1.0f - coverage) + target_op * coverage;
                    glyph.opacity = std::clamp(glyph.opacity, 0.0f, 1.0f);
                }

                // Fill Color
                if (animator.properties.fill_color_value.has_value() || !animator.properties.fill_color_keyframes.empty()) {
                    glyph.fill_color = blend_color(glyph.fill_color, fill, coverage);
                }

                // Stroke Color
                if (animator.properties.stroke_color_value.has_value() || !animator.properties.stroke_color_keyframes.empty()) {
                    glyph.stroke_color = blend_color(glyph.stroke_color, stroke, coverage);
                }

                 // Stroke Width
                if (animator.properties.stroke_width_value.has_value() || !animator.properties.stroke_width_keyframes.empty()) {
                    glyph.stroke_width = glyph.stroke_width * (1.0f - coverage) + static_cast<float>(stroke_width) * coverage;
                }

                // Blur Radius
                if (animator.properties.blur_radius_value.has_value() || !animator.properties.blur_radius_keyframes.empty()) {
                    glyph.blur_radius = glyph.blur_radius * (1.0f - coverage) + static_cast<float>(blur_radius) * coverage;
                    glyph.blur_radius = std::max(0.0f, glyph.blur_radius);
                }

                // Reveal Factor (0.0 = hidden, 1.0 = fully revealed)
                if (animator.properties.reveal_value.has_value() || !animator.properties.reveal_keyframes.empty()) {
                    glyph.reveal_factor = glyph.reveal_factor * (1.0f - coverage) + static_cast<float>(reveal) * coverage;
                    glyph.reveal_factor = std::clamp(glyph.reveal_factor, 0.0f, 1.0f);
                }
            }
        }
    }

private:
    static ::tachyon::ColorSpec blend_color(const ::tachyon::ColorSpec& a, const ::tachyon::ColorSpec& b, float t) {
        float clamped = std::clamp(t, 0.0f, 1.0f);
        auto lerp = [clamped](std::uint8_t x, std::uint8_t y) {
            return static_cast<std::uint8_t>(x + (y - x) * clamped);
        };
        return {lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a)};
    }
};

} // namespace tachyon::text
