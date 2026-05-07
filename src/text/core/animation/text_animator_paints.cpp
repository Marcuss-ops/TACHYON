#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/text/animation/text_anim_backend.h"
#include "tachyon/text/animation/text_anim_migration.h"
#include "tachyon/text/fonts/core/font.h"
#include <algorithm>
#include <cmath>
#include <vector>

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

TextAnimatorContext make_context_from_precomp(
    std::size_t glyph_index,
    float time,
    float total_glyphs,
    float total_clusters,
    float total_words,
    float total_lines,
    float total_non_space_glyphs,
    const PrecompGlyphCtx& precomp,
    const PositionedGlyph& glyph) {

    TextAnimatorContext ctx;
    ctx.glyph_index = glyph_index;
    ctx.time = time;
    ctx.total_glyphs = total_glyphs;
    ctx.total_clusters = total_clusters;
    ctx.total_words = total_words;
    ctx.total_lines = total_lines;
    ctx.total_non_space_glyphs = total_non_space_glyphs;
    ctx.cluster_index = precomp.cluster_index;
    ctx.word_index = precomp.word_index;
    ctx.line_index = precomp.line_index;
    ctx.non_space_glyph_index = precomp.non_space_index;
    ctx.is_space = precomp.is_space || glyph.whitespace;
    ctx.is_rtl = precomp.is_rtl;
    ctx.cluster_codepoint_start = precomp.cluster_codepoint_start;
    ctx.cluster_codepoint_count = precomp.cluster_codepoint_count;
    return ctx;
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

std::vector<PrecompGlyphCtx> precompute_glyph_contexts(const TextLayoutResult& layout) {
    std::vector<PrecompGlyphCtx> precomp;
    precomp.reserve(layout.glyphs.size());
    
    std::size_t cum_non_space = 0;
    for (std::size_t i = 0; i < layout.glyphs.size(); ++i) {
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
    
    return precomp;
}

TextAnimatorContext compute_base_context(const TextLayoutResult& layout) {
    TextAnimatorContext ctx;
    ctx.total_glyphs = static_cast<float>(layout.glyphs.size());
    
    float max_cluster = 0.0f;
    float max_word = 0.0f;
    for (const auto& glyph : layout.glyphs) {
        max_cluster = std::max(max_cluster, static_cast<float>(glyph.cluster_index + 1));
        max_word = std::max(max_word, static_cast<float>(glyph.word_index + 1));
    }
    
    ctx.total_clusters = max_cluster;
    ctx.total_words = max_word;
    ctx.total_lines = static_cast<float>(layout.lines.size());
    
    return ctx;
}

} // namespace

void apply_text_animators(
    TextLayoutResult& layout,
    const TextAnimationOptions& animation) {

    auto animators_span = animation.animators;

    if (!animation.enabled || layout.glyphs.empty()) {
        return;
    }

    // Apply legacy opacity drop as pre-processing (index-based, can't be expressed as animator spec)
    if (animation.per_glyph_opacity_drop > 0.0f) {
        apply_legacy_opacity_drop(layout, animation.per_glyph_opacity_drop);
    }

    if (animators_span.empty()) {
        return;
    }

    const float t = animation.time_seconds;

    const auto base_ctx = compute_base_context(layout);
    const auto precomp = precompute_glyph_contexts(layout);
    
    auto backend = TextAnimExecutionBackend::create_best();

    for (const auto& animator : animators_span) {
        const bool has_tracking = animator.properties.tracking_amount_value.has_value() || 
                                  !animator.properties.tracking_amount_keyframes.empty();
        
        if (has_tracking) {
            float accumulated_tracking = 0.0f;
            for (std::size_t i = 0; i < layout.glyphs.size(); ++i) {
                auto& glyph = layout.glyphs[i];
                const float staggered_t = std::max(0.0f, t - 
                    (animator.selector.stagger_mode == "character" ? 
                        static_cast<float>(i) : static_cast<float>(precomp[i].word_index)) * 
                    static_cast<float>(animator.selector.stagger_delay));
                
                const TextAnimatorContext ctx = make_context_from_precomp(
                    i, staggered_t, base_ctx.total_glyphs, base_ctx.total_clusters,
                    base_ctx.total_words, base_ctx.total_lines, base_ctx.total_non_space_glyphs,
                    precomp[i], glyph);
                
                const float coverage = compute_coverage(animator.selector, ctx);
                if (coverage > 0.0f) {
                    const double tracking_value = sample_scalar_kfs(
                        animator.properties.tracking_amount_value, 
                        animator.properties.tracking_amount_keyframes, 
                        staggered_t);
                    accumulated_tracking += static_cast<float>(tracking_value) * coverage;
                }
                glyph.x += static_cast<std::int32_t>(std::lround(accumulated_tracking));
            }
        }

        const auto plan = ResolvedTextAnimPlan::resolve(animator);
        
        backend->apply(
            plan,
            layout.glyphs,
            animator,
            base_ctx,
            std::span<const PrecompGlyphCtx>(precomp.data(), precomp.size()),
            t,
            static_cast<float>(animator.selector.stagger_delay),
            animator.selector.stagger_mode
        );
    }
}

std::vector<ResolvedGlyphPaint> resolve_glyph_paints(
    const BitmapFont& font,
    const TextLayoutResult& layout,
    const TextAnimationOptions& animation,
    std::span<const TextAnimatorSpec> animators) {

    TextLayoutResult animated_layout = layout;
    apply_text_animators(animated_layout, animation);

    const auto active_animators = animators.empty() ? animation.animators : animators;

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
        
        float width_f = static_cast<float>(glyph->width) * positioned.scale.x;
        std::int32_t width_i = static_cast<std::int32_t>(std::lround(width_f));
        paint.target_width = static_cast<std::uint32_t>(std::max(static_cast<std::int32_t>(1), width_i));
        
        float height_f = static_cast<float>(glyph->height) * positioned.scale.y;
        std::int32_t height_i = static_cast<std::int32_t>(std::lround(height_f));
        paint.target_height = static_cast<std::uint32_t>(std::max(static_cast<std::int32_t>(1), height_i));
        
        paint.opacity = std::clamp(positioned.opacity * positioned.reveal_factor, kZero, kOne);
        paint.fill_color = positioned.fill_color;
        paint.stroke_color = positioned.stroke_color;
        paint.stroke_width = positioned.stroke_width;
        paint.tracking_offset = static_cast<float>(positioned.x - layout.glyphs[idx].x);
        paint.glyph_index = positioned.glyph_index;

        if (animation.motion_blur_intensity > 0.0f) {
            paint.motion_blur_vector.x = 0.0f;
            paint.motion_blur_vector.y = 0.0f;
            
            for (const auto& animator : active_animators) {
                if (!animator.properties.position_offset_keyframes.empty()) {
                    paint.motion_blur_vector.y += 5.0f * animation.motion_blur_intensity;
                }
            }
        }

        paints.push_back(paint);
    }

    for (const auto& animator : active_animators) {
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
                    c_paint.fill_color = animator.cursor.color_override.value_or(
                        !paints.empty() ? paints.back().fill_color : ::tachyon::ColorSpec{255, 255, 255, 255});
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
