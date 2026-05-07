#include "tachyon/text/animation/text_anim_backend.h"
#include "tachyon/text/animation/text_anim_property_registry.h"
#include "tachyon/text/animation/text_animator_utils.h"
#include <algorithm>
#include <cmath>

#ifdef TACHYON_AVX2
#include <immintrin.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

namespace tachyon::text {

static TextAnimatorContext make_context_from_precomp(
    std::size_t glyph_index,
    float t,
    float total_glyphs,
    float total_clusters,
    float total_words,
    float total_lines,
    float total_non_space_glyphs,
    const PrecompGlyphCtx& precomp,
    const PositionedGlyph& glyph) {

    TextAnimatorContext ctx;
    ctx.glyph_index = glyph_index;
    ctx.time = t;
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

void ScalarTextAnimBackend::apply(
    const ResolvedTextAnimPlan& plan,
    std::span<PositionedGlyph> glyphs,
    const TextAnimatorSpec& animator,
    const TextAnimatorContext& base_ctx,
    std::span<const PrecompGlyphCtx> precomp,
    float t,
    float stagger_delay,
    const std::string& stagger_mode) {

    const bool is_char_stagger = (stagger_mode == "character");
    const std::size_t total_glyphs = glyphs.size();
    
    for (std::size_t i = 0; i < total_glyphs; ++i) {
        auto& glyph = glyphs[i];
        const float staggered_t = std::max(0.0f, t - 
            (is_char_stagger ? static_cast<float>(i) : static_cast<float>(precomp[i].word_index)) * 
            stagger_delay);
        
        const TextAnimatorContext ctx = make_context_from_precomp(
            i, staggered_t, base_ctx.total_glyphs, base_ctx.total_clusters,
            base_ctx.total_words, base_ctx.total_lines, base_ctx.total_non_space_glyphs,
            precomp[i], glyph);
        
        const float coverage = compute_coverage(animator.selector, ctx);
        if (coverage <= 0.0f) continue;
        
        for (const auto& resolved_prop : plan.resolved_properties) {
            resolved_prop.apply_to_glyph(glyph, staggered_t, coverage);
        }
    }
}

void OpenMpTextAnimBackend::apply(
    const ResolvedTextAnimPlan& plan,
    std::span<PositionedGlyph> glyphs,
    const TextAnimatorSpec& animator,
    const TextAnimatorContext& base_ctx,
    std::span<const PrecompGlyphCtx> precomp,
    float t,
    float stagger_delay,
    const std::string& stagger_mode) {

    const bool is_char_stagger = (stagger_mode == "character");
    const std::size_t total_glyphs = glyphs.size();
    
#ifdef _OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int i = 0; i < static_cast<int>(total_glyphs); ++i) {
        std::size_t idx = static_cast<std::size_t>(i);
        auto& glyph = glyphs[idx];
        const float staggered_t = std::max(0.0f, t - 
            (is_char_stagger ? static_cast<float>(idx) : static_cast<float>(precomp[idx].word_index)) * 
            stagger_delay);
        
        const TextAnimatorContext ctx = make_context_from_precomp(
            idx, staggered_t, base_ctx.total_glyphs, base_ctx.total_clusters,
            base_ctx.total_words, base_ctx.total_lines, base_ctx.total_non_space_glyphs,
            precomp[idx], glyph);
        
        const float coverage = compute_coverage(animator.selector, ctx);
        if (coverage <= 0.0f) continue;
        
        for (const auto& resolved_prop : plan.resolved_properties) {
            resolved_prop.apply_to_glyph(glyph, staggered_t, coverage);
        }
    }
}

#ifdef TACHYON_AVX2
void Avx2TextAnimBackend::apply(
    const ResolvedTextAnimPlan& plan,
    std::span<PositionedGlyph> glyphs,
    const TextAnimatorSpec& animator,
    const TextAnimatorContext& base_ctx,
    std::span<const PrecompGlyphCtx> precomp,
    float t,
    float stagger_delay,
    const std::string& stagger_mode) {

    const int n = static_cast<int>(glyphs.size());
    const int n8 = n & ~7;
    
    const bool is_char_stagger = (stagger_mode == "character");
    
    const float sel_start = static_cast<float>(animator.selector.start) / 100.0f;
    const float sel_end = static_cast<float>(animator.selector.end) / 100.0f;
    const float sel_span = sel_end - sel_start;
    
    const __m256 v_one = _mm256_set1_ps(1.0f);
    const __m256 v_zero = _mm256_set1_ps(0.0f);
    const __m256 v_t = _mm256_set1_ps(t);
    const __m256 v_stagger_delay = _mm256_set1_ps(stagger_delay);
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
        
        const __m256 v_staggered_t = _mm256_max_ps(v_zero, _mm256_sub_ps(v_t, _mm256_mul_ps(v_idx, v_stagger_delay)));
        
        const float inv_total = 1.0f / std::max(1.0f, static_cast<float>(glyphs.size() - 1));
        __m256 v_t_norm = _mm256_mul_ps(v_idx, _mm256_set1_ps(inv_total));
        __m256 v_coverage = _mm256_min_ps(v_one, _mm256_max_ps(v_zero, _mm256_mul_ps(_mm256_sub_ps(v_t_norm, v_sel_start), v_sel_inv_span)));
        
        float* cov_ptr = (float*)&v_coverage;
        float* t_ptr = (float*)&v_staggered_t;
        for (int j = 0; j < 8; ++j) {
            float cov = cov_ptr[j];
            if (cov <= 0.0f) continue;
            
            auto& g = glyphs[i + j];
            float staggered_t = t_ptr[j];
            for (const auto& resolved_prop : plan.resolved_properties) {
                resolved_prop.apply_to_glyph(g, staggered_t, cov);
            }
        }
    }
    
    for (std::size_t i = static_cast<std::size_t>(n8); i < glyphs.size(); ++i) {
        auto& glyph = glyphs[i];
        const float staggered_t = std::max(0.0f, t - 
            (is_char_stagger ? static_cast<float>(i) : static_cast<float>(precomp[i].word_index)) * 
            stagger_delay);
        
        const TextAnimatorContext ctx = make_context_from_precomp(
            i, staggered_t, base_ctx.total_glyphs, base_ctx.total_clusters,
            base_ctx.total_words, base_ctx.total_lines, base_ctx.total_non_space_glyphs,
            precomp[i], glyph);
        
        const float coverage = compute_coverage(animator.selector, ctx);
        if (coverage <= 0.0f) continue;
        
        for (const auto& resolved_prop : plan.resolved_properties) {
            resolved_prop.apply_to_glyph(glyph, staggered_t, coverage);
        }
    }
}
#endif

std::unique_ptr<TextAnimExecutionBackend> TextAnimExecutionBackend::create_scalar() {
    return std::make_unique<ScalarTextAnimBackend>();
}

std::unique_ptr<TextAnimExecutionBackend> TextAnimExecutionBackend::create_openmp() {
    return std::make_unique<OpenMpTextAnimBackend>();
}

std::unique_ptr<TextAnimExecutionBackend> TextAnimExecutionBackend::create_avx2() {
#ifdef TACHYON_AVX2
    return std::make_unique<Avx2TextAnimBackend>();
#else
    return create_scalar();
#endif
}

std::unique_ptr<TextAnimExecutionBackend> TextAnimExecutionBackend::create_best() {
#ifdef TACHYON_AVX2
    return create_avx2();
#else
#ifdef _OPENMP
    return create_openmp();
#else
    return create_scalar();
#endif
#endif
}

} // namespace tachyon::text
