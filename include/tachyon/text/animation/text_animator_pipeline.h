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
            
            // Sample the scalar/vector/color properties at time t
            double opacity = sample_scalar_kfs(animator.properties.opacity_value, animator.properties.opacity_keyframes, t);
            math::Vector2 pos_offset = sample_vector2_kfs(animator.properties.position_offset_value, animator.properties.position_offset_keyframes, t);
            double scale = sample_scalar_kfs(animator.properties.scale_value, animator.properties.scale_keyframes, t);
            double rotation = sample_scalar_kfs(animator.properties.rotation_value, animator.properties.rotation_keyframes, t);
            double tracking = sample_scalar_kfs(animator.properties.tracking_amount_value, animator.properties.tracking_amount_keyframes, t);
            ::tachyon::ColorSpec fill = sample_color_kfs(animator.properties.fill_color_value, animator.properties.fill_color_keyframes, t);
            ::tachyon::ColorSpec stroke = sample_color_kfs(animator.properties.stroke_color_value, animator.properties.stroke_color_keyframes, t);
            double stroke_width = sample_scalar_kfs(animator.properties.stroke_width_value, animator.properties.stroke_width_keyframes, t);

            float accumulated_tracking = 0.0f;

            for (std::size_t i = 0; i < num_glyphs; ++i) {
                auto& glyph = layout.glyphs[i];

                // 1. Evaluate Selector Coverage [0, 1]
                TextAnimatorContext ctx;
                ctx.glyph_index = i;
                ctx.cluster_index = glyph.cluster_index;
                ctx.total_glyphs = static_cast<float>(num_glyphs);
                ctx.time = t;
                
                float coverage = compute_coverage(animator.selector, ctx);
                
                // Accumulate tracking even if coverage is 0 for this glyph, if previous glyphs had tracking
                // Actually tracking applies to advance, pushing subsequent glyphs.
                if (coverage > 0.0f) {
                    accumulated_tracking += static_cast<float>(tracking) * coverage;
                }

                glyph.position.x += accumulated_tracking;

                if (coverage <= 0.0f) continue;

                // 2. Apply animated properties scaled by coverage
                
                // Position Offset
                glyph.position.x += pos_offset.x * coverage;
                glyph.position.y += pos_offset.y * coverage;

                // Scale
                float target_scale = static_cast<float>(scale);
                if (target_scale != 1.0f || animator.properties.scale_value.has_value() || !animator.properties.scale_keyframes.empty()) {
                    glyph.scale.x = glyph.scale.x * (1.0f - coverage) + (glyph.scale.x * target_scale) * coverage;
                    glyph.scale.y = glyph.scale.y * (1.0f - coverage) + (glyph.scale.y * target_scale) * coverage;
                }

                // Rotation
                glyph.rotation += static_cast<float>(rotation) * coverage;

                // Opacity
                if (animator.properties.opacity_value.has_value() || !animator.properties.opacity_keyframes.empty()) {
                    float target_op = static_cast<float>(opacity);
                    glyph.opacity = glyph.opacity * (1.0f - coverage) + target_op * coverage;
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
