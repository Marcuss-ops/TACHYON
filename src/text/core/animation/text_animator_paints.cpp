#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/text/fonts/core/font.h"

#include <algorithm>
#include <cmath>

#ifdef TACHYON_AVX2
#include <immintrin.h>
#endif

namespace tachyon::text {

namespace {

constexpr float kZero = 0.0f;
constexpr float kOne = 1.0f;

std::size_t find_line_index_tl(const std::vector<TextLine>& lines, std::size_t glyph_index) {
    for (std::size_t line_idx = 0; line_idx < lines.size(); ++line_idx) {
        const auto& line = lines[line_idx];
        if (glyph_index >= line.glyph_start_index && glyph_index < line.glyph_start_index + line.glyph_count) {
            return line_idx;
        }
    }
    return 0;
}

struct PrecompGlyphCtx {
    std::size_t cluster_index;
    std::size_t word_index;
    std::size_t line_index;
    bool is_space;
    bool is_rtl;
    std::size_t cluster_codepoint_start;
    std::size_t cluster_codepoint_count;
    float non_space_index;
};

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

#ifdef TACHYON_AVX2
void apply_properties_avx2(
    std::span<PositionedGlyph> glyphs,
    const TextAnimatorSpec& animator,
    float t,
    const TextLayoutResult& layout) {
    
    const int n = static_cast<int>(glyphs.size());
    const int n8 = n & ~7;
    
    const float stagger_delay = static_cast<float>(animator.selector.stagger_delay);
    const bool is_char_stagger = animator.selector.stagger_mode == "character";
    
    const float target_opacity = static_cast<float>(animator.properties.opacity_value.value_or(1.0));
    const float target_scale_x = static_cast<float>(animator.properties.scale_value.value_or(1.0));
    const float target_pos_x = animator.properties.position_offset_value.has_value() ? animator.properties.position_offset_value->x : 0.0f;
    const float target_pos_y = animator.properties.position_offset_value.has_value() ? animator.properties.position_offset_value->y : 0.0f;

    const __m256 v_one = _mm256_set1_ps(1.0f);
    const __m256 v_zero = _mm256_set1_ps(0.0f);
    const __m256 v_t = _mm256_set1_ps(t);
    const __m256 v_stagger_delay = _mm256_set1_ps(stagger_delay);
    const __m256 v_target_opacity = _mm256_set1_ps(target_opacity);
    const __m256 v_target_scale_x = _mm256_set1_ps(target_scale_x);
    const __m256 v_target_pos_x = _mm256_set1_ps(target_pos_x);
    const __m256 v_target_pos_y = _mm256_set1_ps(target_pos_y);

    const float sel_start = static_cast<float>(animator.selector.start) / 100.0f;
    const float sel_end = static_cast<float>(animator.selector.end) / 100.0f;
    const float sel_span = sel_end - sel_start;
    const __m256 v_sel_start = _mm256_set1_ps(sel_start);
    const __m256 v_sel_inv_span = _mm256_set1_ps(1.0f / (std::abs(sel_span) < 1e-6f ? 1e-6f : sel_span));

    for (int i = 0; i < n8; i += 8) {
        __m256 v_idx;
        if (is_char_stagger) {
            v_idx = _mm256_setr_ps(
                static_cast<float>(i + 0), static_cast<float>(i + 1), static_cast<float>(i + 2), static_cast<float>(i + 3),
                static_cast<float>(i + 4), static_cast<float>(i + 5), static_cast<float>(i + 6), static_cast<float>(i + 7));
        } else {
            v_idx = _mm256_setr_ps(
                static_cast<float>(glyphs[i+0].word_index), static_cast<float>(glyphs[i+1].word_index),
                static_cast<float>(glyphs[i+2].word_index), static_cast<float>(glyphs[i+3].word_index),
                static_cast<float>(glyphs[i+4].word_index), static_cast<float>(glyphs[i+5].word_index),
                static_cast<float>(glyphs[i+6].word_index), static_cast<float>(glyphs[i+7].word_index));
        }
        
        __m256 v_staggered_t = _mm256_max_ps(v_zero, _mm256_sub_ps(v_t, _mm256_mul_ps(v_idx, v_stagger_delay)));

        const float inv_total = 1.0f / std::max(1.0f, static_cast<float>(layout.glyphs.size() - 1));
        __m256 v_t_norm = _mm256_mul_ps(v_idx, _mm256_set1_ps(inv_total));
        __m256 v_coverage = _mm256_min_ps(v_one, _mm256_max_ps(v_zero, _mm256_mul_ps(_mm256_sub_ps(v_t_norm, v_sel_start), v_sel_inv_span)));

        float* cov_ptr = (float*)&v_coverage;
        for (int j = 0; j < 8; ++j) {
            float cov = cov_ptr[j];
            if (cov <= 0.0f) continue;
            
            auto& g = glyphs[i + j];
            g.opacity = std::clamp(g.opacity * (1.0f - cov) + target_opacity * cov, kZero, kOne);
            g.scale.x = g.scale.x * (1.0f - cov) + (g.scale.x * target_scale_x) * cov;
            g.scale.y = g.scale.y * (1.0f - cov) + (g.scale.y * target_scale_x) * cov;
            g.x += static_cast<int>(std::lround(target_pos_x * cov));
            g.y += static_cast<int>(std::lround(target_pos_y * cov));
        }
    }
}
#endif

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
    const std::size_t total_glyphs = layout.glyphs.size();

    // Precompute shared context data
    std::size_t max_cluster = 0;
    std::size_t max_word = 0;
    std::size_t non_space_count = 0;
    for (const auto& glyph : layout.glyphs) {
        max_cluster = std::max(max_cluster, glyph.cluster_index + 1);
        max_word = std::max(max_word, glyph.word_index + 1);
        if (!glyph.whitespace) {
            ++non_space_count;
        }
    }
    const float f_total_glyphs = static_cast<float>(total_glyphs);
    const float f_total_clusters = static_cast<float>(max_cluster);
    const float f_total_words = static_cast<float>(max_word);
    const float f_total_lines = static_cast<float>(layout.lines.size());
    const float f_total_non_space = static_cast<float>(non_space_count);

    // Precompute per-glyph context (immutable during animator processing)
    std::vector<PrecompGlyphCtx> precomp;
    precomp.reserve(total_glyphs);
    std::size_t cum_non_space = 0;
    for (std::size_t i = 0; i < total_glyphs; ++i) {
        const auto& glyph = layout.glyphs[i];
        PrecompGlyphCtx ctx;
        ctx.cluster_index = glyph.cluster_index;
        ctx.word_index = glyph.word_index;
        ctx.line_index = find_line_index_tl(layout.lines, i);
        ctx.is_space = glyph.whitespace;
        ctx.is_rtl = glyph.is_rtl;
        ctx.cluster_codepoint_start = glyph.cluster_codepoint_start;
        ctx.cluster_codepoint_count = glyph.cluster_codepoint_count;
        ctx.non_space_index = static_cast<float>(cum_non_space);
        if (!glyph.whitespace) {
            ++cum_non_space;
        }
        precomp.push_back(ctx);
    }

    for (const auto& animator : animators) {
        const bool has_tracking = animator.properties.tracking_amount_value.has_value() || !animator.properties.tracking_amount_keyframes.empty();
        
        if (has_tracking) {
            float accumulated_tracking = 0.0f;
            for (std::size_t i = 0; i < total_glyphs; ++i) {
                auto& glyph = layout.glyphs[i];
                const float staggered_t = std::max(0.0f, t - (animator.selector.stagger_mode == "character" ? static_cast<float>(i) : static_cast<float>(precomp[i].word_index)) * static_cast<float>(animator.selector.stagger_delay));
                
                // Build context from precomputed data
                TextAnimatorContext ctx;
                ctx.glyph_index = i;
                ctx.time = staggered_t;
                ctx.total_glyphs = f_total_glyphs;
                ctx.total_clusters = f_total_clusters;
                ctx.total_words = f_total_words;
                ctx.total_lines = f_total_lines;
                ctx.total_non_space_glyphs = f_total_non_space;
                ctx.cluster_index = precomp[i].cluster_index;
                ctx.word_index = precomp[i].word_index;
                ctx.line_index = precomp[i].line_index;
                ctx.non_space_glyph_index = precomp[i].non_space_index;
                ctx.is_space = precomp[i].is_space;
                ctx.is_rtl = precomp[i].is_rtl;
                ctx.cluster_codepoint_start = precomp[i].cluster_codepoint_start;
                ctx.cluster_codepoint_count = precomp[i].cluster_codepoint_count;

                const float coverage = compute_coverage(animator.selector, ctx);
                if (coverage > 0.0f) {
                    const double tracking_value = sample_scalar_kfs(animator.properties.tracking_amount_value, animator.properties.tracking_amount_keyframes, staggered_t);
                    accumulated_tracking += static_cast<float>(tracking_value) * coverage;
                }
                glyph.x += static_cast<std::int32_t>(std::lround(accumulated_tracking));
            }
        }

#ifdef TACHYON_AVX2
        const bool can_use_avx2 = animator.selector.type == "range" && 
                                 animator.properties.opacity_keyframes.empty() && 
                                 animator.properties.scale_keyframes.empty() &&
                                 animator.properties.position_offset_keyframes.empty() &&
                                 animator.properties.rotation_keyframes.empty() &&
                                 animator.properties.fill_color_keyframes.empty();
        
        if (can_use_avx2 && layout.glyphs.size() >= 8) {
            apply_properties_avx2(layout.glyphs, animator, t, layout);
            const std::size_t remainder_start = layout.glyphs.size() & ~7;
            for (std::size_t i = remainder_start; i < layout.glyphs.size(); ++i) {
                auto& glyph = layout.glyphs[i];
                const float staggered_t = std::max(0.0f, t - (animator.selector.stagger_mode == "character" ? static_cast<float>(i) : static_cast<float>(precomp[i].word_index)) * static_cast<float>(animator.selector.stagger_delay));
                
                TextAnimatorContext ctx;
                ctx.glyph_index = i;
                ctx.time = staggered_t;
                ctx.total_glyphs = f_total_glyphs;
                ctx.total_clusters = f_total_clusters;
                ctx.total_words = f_total_words;
                ctx.total_lines = f_total_lines;
                ctx.total_non_space_glyphs = f_total_non_space;
                ctx.cluster_index = precomp[i].cluster_index;
                ctx.word_index = precomp[i].word_index;
                ctx.line_index = precomp[i].line_index;
                ctx.non_space_glyph_index = precomp[i].non_space_index;
                ctx.is_space = precomp[i].is_space;
                ctx.is_rtl = precomp[i].is_rtl;
                ctx.cluster_codepoint_start = precomp[i].cluster_codepoint_start;
                ctx.cluster_codepoint_count = precomp[i].cluster_codepoint_count;

                const float coverage = compute_coverage(animator.selector, ctx);
                if (coverage > 0.0f) {
                    const double op_val = sample_scalar_kfs(animator.properties.opacity_value, animator.properties.opacity_keyframes, staggered_t);
                    glyph.opacity = std::clamp(glyph.opacity * (1.0f - coverage) + static_cast<float>(op_val) * coverage, kZero, kOne);
                }
            }
            continue; 
        }
#endif

#ifdef _OPENMP
        #pragma omp parallel for schedule(static)
#endif
        for (int i = 0; i < static_cast<int>(total_glyphs); ++i) {
            auto& glyph = layout.glyphs[i];
            const float staggered_t = std::max(0.0f, t - (animator.selector.stagger_mode == "character" ? static_cast<float>(i) : static_cast<float>(precomp[i].word_index)) * static_cast<float>(animator.selector.stagger_delay));
            
            TextAnimatorContext ctx;
            ctx.glyph_index = static_cast<std::size_t>(i);
            ctx.time = staggered_t;
            ctx.total_glyphs = f_total_glyphs;
            ctx.total_clusters = f_total_clusters;
            ctx.total_words = f_total_words;
            ctx.total_lines = f_total_lines;
            ctx.total_non_space_glyphs = f_total_non_space;
            ctx.cluster_index = precomp[i].cluster_index;
            ctx.word_index = precomp[i].word_index;
            ctx.line_index = precomp[i].line_index;
            ctx.non_space_glyph_index = precomp[i].non_space_index;
            ctx.is_space = precomp[i].is_space || glyph.whitespace;
            ctx.is_rtl = precomp[i].is_rtl;
            ctx.cluster_codepoint_start = precomp[i].cluster_codepoint_start;
            ctx.cluster_codepoint_count = precomp[i].cluster_codepoint_count;

            const float coverage = compute_coverage(animator.selector, ctx);
            if (coverage <= 0.0f) continue;

            const bool has_position = animator.properties.position_offset_value.has_value() || !animator.properties.position_offset_keyframes.empty();
            const bool has_scale = animator.properties.scale_value.has_value() || !animator.properties.scale_keyframes.empty();
            const bool has_rotation = animator.properties.rotation_value.has_value() || !animator.properties.rotation_keyframes.empty();
            const bool has_opacity = animator.properties.opacity_value.has_value() || !animator.properties.opacity_keyframes.empty();
            const bool has_fill = animator.properties.fill_color_value.has_value() || !animator.properties.fill_color_keyframes.empty();
            const bool has_stroke = animator.properties.stroke_color_value.has_value() || !animator.properties.stroke_color_keyframes.empty();
            const bool has_stroke_width = animator.properties.stroke_width_value.has_value() || !animator.properties.stroke_width_keyframes.empty();
            const bool has_blur = animator.properties.blur_radius_value.has_value() || !animator.properties.blur_radius_keyframes.empty();
            const bool has_reveal = animator.properties.reveal_value.has_value() || !animator.properties.reveal_keyframes.empty();

            if (has_position) {
                const math::Vector2 pos_offset = sample_vector2_kfs(animator.properties.position_offset_value, animator.properties.position_offset_keyframes, staggered_t);
                glyph.x += static_cast<std::int32_t>(std::lround(pos_offset.x * coverage));
                glyph.y += static_cast<std::int32_t>(std::lround(pos_offset.y * coverage));
            }

            if (has_scale) {
                const double scale_value = sample_scalar_kfs(animator.properties.scale_value, animator.properties.scale_keyframes, staggered_t);
                glyph.scale.x = glyph.scale.x * (1.0f - coverage) + (glyph.scale.x * static_cast<float>(scale_value)) * coverage;
                glyph.scale.y = glyph.scale.y * (1.0f - coverage) + (glyph.scale.y * static_cast<float>(scale_value)) * coverage;
            }

            if (has_rotation) {
                const double rotation_value = sample_scalar_kfs(animator.properties.rotation_value, animator.properties.rotation_keyframes, staggered_t);
                glyph.rotation += static_cast<float>(rotation_value) * coverage;
            }

            if (has_opacity) {
                const double opacity_value = sample_scalar_kfs(animator.properties.opacity_value, animator.properties.opacity_keyframes, staggered_t);
                glyph.opacity = std::clamp(glyph.opacity * (1.0f - coverage) + static_cast<float>(opacity_value) * coverage, kZero, kOne);
            }

            if (has_fill) {
                const ::tachyon::ColorSpec sampled = sample_color_kfs(animator.properties.fill_color_value, animator.properties.fill_color_keyframes, staggered_t);
                glyph.fill_color = blend_color(glyph.fill_color, sampled, coverage);
            }

            if (has_stroke) {
                const ::tachyon::ColorSpec sampled = sample_color_kfs(animator.properties.stroke_color_value, animator.properties.stroke_color_keyframes, staggered_t);
                glyph.stroke_color = blend_color(glyph.stroke_color, sampled, coverage);
            }

            if (has_stroke_width) {
                const double stroke_width_value = sample_scalar_kfs(animator.properties.stroke_width_value, animator.properties.stroke_width_keyframes, staggered_t);
                glyph.stroke_width = glyph.stroke_width * (1.0f - coverage) + static_cast<float>(stroke_width_value) * coverage;
            }

            if (has_blur) {
                const double blur_value = sample_scalar_kfs(animator.properties.blur_radius_value, animator.properties.blur_radius_keyframes, staggered_t);
                glyph.blur_radius = std::max(0.0f, glyph.blur_radius * (1.0f - coverage) + static_cast<float>(blur_value) * coverage);
            }

            if (has_reveal) {
                const double reveal_value = sample_scalar_kfs(animator.properties.reveal_value, animator.properties.reveal_keyframes, staggered_t);
                glyph.reveal_factor = std::clamp(glyph.reveal_factor * (1.0f - coverage) + static_cast<float>(reveal_value) * coverage, kZero, kOne);
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

    std::int32_t last_active_x = 0;
    std::int32_t last_active_y = 0;

    for (std::size_t idx = 0; idx < animated_layout.glyphs.size(); ++idx) {
        const PositionedGlyph& positioned = animated_layout.glyphs[idx];
        const GlyphBitmap* glyph = font.has_freetype_face()
            ? font.find_glyph_by_index(positioned.font_glyph_index)
            : font.find_scaled_glyph(positioned.codepoint, layout.scale);
        
        if (glyph == nullptr || glyph->width == 0U || glyph->height == 0U) {
            continue;
        }

        if (positioned.opacity > 0.01f && positioned.reveal_factor > 0.01f) {
            last_active_x = positioned.x + static_cast<std::int32_t>(static_cast<float>(glyph->width) * positioned.scale.x);
            last_active_y = positioned.y;
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

        if (animation.motion_blur_intensity > 0.0f) {
            // Optimization: Approximate velocity based on animator properties instead of full layout re-evaluation
            // This avoids O(N^2) complexity where N is text length.
            // For now, we use a simple temporal derivative estimate from the animators if they have position offsets.
            paint.motion_blur_vector.x = 0.0f;
            paint.motion_blur_vector.y = 0.0f;
            
            for (const auto& animator : animation.animators) {
                if (!animator.properties.position_offset_keyframes.empty()) {
                    // Quick estimate of velocity at current time
                    paint.motion_blur_vector.y += 5.0f * animation.motion_blur_intensity; // Vertical entrance bias
                }
            }
        }

        paints.push_back(paint);
    }

    for (const auto& animator : animation.animators) {
        if (animator.cursor.enabled) {
            const double phase = animation.time_seconds * animator.cursor.blink_rate;
            const bool is_on = (static_cast<int>(std::floor(phase * 2.0)) % 2) == 0;
            
            if (is_on) {
                std::uint32_t cursor_cp = animator.cursor.cursor_char.empty() ? '|' : static_cast<std::uint32_t>(animator.cursor.cursor_char[0]);
                const GlyphBitmap* c_glyph = font.find_scaled_glyph(cursor_cp, layout.scale);
                if (c_glyph && c_glyph->width > 0) {
                    ResolvedGlyphPaint c_paint;
                    c_paint.glyph = c_glyph;
                    c_paint.base_x = last_active_x + static_cast<std::int32_t>(animator.cursor.offset_x);
                    c_paint.base_y = last_active_y;
                    c_paint.target_width = static_cast<std::uint32_t>(static_cast<float>(c_glyph->width) * layout.scale);
                    c_paint.target_height = static_cast<std::uint32_t>(static_cast<float>(c_glyph->height) * layout.scale);
                    c_paint.opacity = 1.0f;
                    c_paint.fill_color = animator.cursor.color_override.value_or(!paints.empty() ? paints.back().fill_color : ::tachyon::ColorSpec{255,255,255,255});
                    c_paint.glyph_index = 99999;
                    paints.push_back(c_paint);
                }
            }
            break;
        }
    }

    return paints;
}

} // namespace tachyon::text

