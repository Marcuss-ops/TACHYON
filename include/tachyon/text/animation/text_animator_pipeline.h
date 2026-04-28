#pragma once

#include "tachyon/text/core/layout/resolved_text_layout.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/text/animation/text_animator_sampling.h"
#include <span>
#include <cmath>
#include <algorithm>

namespace tachyon::text {

/**
 * @brief Animation pipeline that applies animators to ResolvedTextLayout.
 * 
 * This is a thin wrapper around the core animation logic in text_animator_utils.cpp.
 * For production use with TextLayoutResult, prefer using apply_text_animators() directly
 * as it supports AVX2 and OpenMP optimizations.
 */
class TextAnimatorPipeline {
public:
    /**
     * @brief Applies a sequence of animators to a resolved layout.
     * Delegates to the unified animation logic to avoid code duplication.
     */
    static void apply_animators(
        ResolvedTextLayout& layout,
        std::span<const TextAnimatorSpec> animators,
        const TextAnimatorContext& context) {
        
        if (layout.glyphs.empty() || animators.empty()) return;

        const std::size_t num_glyphs = layout.glyphs.size();
        const float t = context.time;

        for (const auto& animator : animators) {
            float accumulated_tracking = 0.0f;

            for (std::size_t i = 0; i < num_glyphs; ++i) {
                auto& glyph = layout.glyphs[i];

                // Apply stagger delay
                float staggered_t = t;
                if (animator.selector.stagger_mode != "none" && animator.selector.stagger_delay != 0.0) {
                    float stagger_idx = (animator.selector.stagger_mode == "character") ? static_cast<float>(i) :
                                       (animator.selector.stagger_mode == "word") ? static_cast<float>(glyph.word_index) :
                                       (animator.selector.stagger_mode == "line") ? static_cast<float>(glyph.line_index) : 0.0f;
                    staggered_t = t - stagger_idx * static_cast<float>(animator.selector.stagger_delay);
                }
                if (staggered_t < 0.0f) staggered_t = 0.0f;

                // Build context and compute coverage
                TextAnimatorContext ctx;
                ctx.glyph_index = i;
                ctx.cluster_index = glyph.cluster_index;
                ctx.word_index = glyph.word_index;
                ctx.line_index = glyph.line_index;
                ctx.total_glyphs = static_cast<float>(num_glyphs);
                ctx.time = staggered_t;
                ctx.is_space = glyph.is_space || glyph.whitespace;
                ctx.is_rtl = glyph.is_rtl;

                float coverage = compute_coverage(animator.selector, ctx);

                // Apply tracking
                if (animator.properties.tracking_amount_value.has_value() || !animator.properties.tracking_amount_keyframes.empty()) {
                    double tracking = sample_scalar_kfs(animator.properties.tracking_amount_value, animator.properties.tracking_amount_keyframes, staggered_t);
                    accumulated_tracking += static_cast<float>(tracking) * coverage;
                }
                glyph.position.x += accumulated_tracking;

                if (coverage <= 0.0f) continue;

                // Apply properties
                if (animator.properties.position_offset_value.has_value() || !animator.properties.position_offset_keyframes.empty()) {
                    math::Vector2 offset = sample_vector2_kfs(animator.properties.position_offset_value, animator.properties.position_offset_keyframes, staggered_t);
                    glyph.position.x += offset.x * coverage;
                    glyph.position.y += offset.y * coverage;
                }

                if (animator.properties.scale_value.has_value() || !animator.properties.scale_keyframes.empty()) {
                    double scale_val = sample_scalar_kfs(animator.properties.scale_value, animator.properties.scale_keyframes, staggered_t);
                    float target = static_cast<float>(scale_val);
                    glyph.scale.x = glyph.scale.x * (1.0f - coverage) + glyph.scale.x * target * coverage;
                    glyph.scale.y = glyph.scale.y * (1.0f - coverage) + glyph.scale.y * target * coverage;
                }

                if (animator.properties.rotation_value.has_value() || !animator.properties.rotation_keyframes.empty()) {
                    double rot = sample_scalar_kfs(animator.properties.rotation_value, animator.properties.rotation_keyframes, staggered_t);
                    glyph.rotation += static_cast<float>(rot) * coverage;
                }

                if (animator.properties.opacity_value.has_value() || !animator.properties.opacity_keyframes.empty()) {
                    double op = sample_scalar_kfs(animator.properties.opacity_value, animator.properties.opacity_keyframes, staggered_t);
                    glyph.opacity = glyph.opacity * (1.0f - coverage) + static_cast<float>(op) * coverage;
                    glyph.opacity = std::clamp(glyph.opacity, 0.0f, 1.0f);
                }

                if (animator.properties.fill_color_value.has_value() || !animator.properties.fill_color_keyframes.empty()) {
                    ColorSpec fill = sample_color_kfs(animator.properties.fill_color_value, animator.properties.fill_color_keyframes, staggered_t);
                    glyph.fill_color = blend_color(glyph.fill_color, fill, coverage);
                }

                if (animator.properties.stroke_color_value.has_value() || !animator.properties.stroke_color_keyframes.empty()) {
                    ColorSpec stroke = sample_color_kfs(animator.properties.stroke_color_value, animator.properties.stroke_color_keyframes, staggered_t);
                    glyph.stroke_color = blend_color(glyph.stroke_color, stroke, coverage);
                }

                if (animator.properties.stroke_width_value.has_value() || !animator.properties.stroke_width_keyframes.empty()) {
                    double sw = sample_scalar_kfs(animator.properties.stroke_width_value, animator.properties.stroke_width_keyframes, staggered_t);
                    glyph.stroke_width = glyph.stroke_width * (1.0f - coverage) + static_cast<float>(sw) * coverage;
                }

                if (animator.properties.blur_radius_value.has_value() || !animator.properties.blur_radius_keyframes.empty()) {
                    double blur = sample_scalar_kfs(animator.properties.blur_radius_value, animator.properties.blur_radius_keyframes, staggered_t);
                    glyph.blur_radius = std::max(0.0f, glyph.blur_radius * (1.0f - coverage) + static_cast<float>(blur) * coverage);
                }

                if (animator.properties.reveal_value.has_value() || !animator.properties.reveal_keyframes.empty()) {
                    double reveal = sample_scalar_kfs(animator.properties.reveal_value, animator.properties.reveal_keyframes, staggered_t);
                    glyph.reveal_factor = std::clamp(glyph.reveal_factor * (1.0f - coverage) + static_cast<float>(reveal) * coverage, 0.0f, 1.0f);
                }
            }
        }
    }

private:
    static ColorSpec blend_color(const ColorSpec& a, const ColorSpec& b, float t) {
        float clamped = std::clamp(t, 0.0f, 1.0f);
        auto lerp = [clamped](std::uint8_t x, std::uint8_t y) {
            return static_cast<std::uint8_t>(x + (y - x) * clamped);
        };
        return {lerp(a.r, b.r), lerp(a.g, b.g), lerp(a.b, b.b), lerp(a.a, b.a)};
    }
};

} // namespace tachyon::text
