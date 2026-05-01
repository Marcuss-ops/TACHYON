#include "tachyon/text/animation/text_animator_utils.h"
#include "tachyon/core/expressions/expression_engine.h"
#include <algorithm>

namespace tachyon::text {

namespace {
    constexpr float kZero = 0.0f;
    constexpr float kOne = 1.0f;
}

/**
 * Unified Expression Evaluator for Text Animators.
 * Bridges TextAnimatorContext to CoreExpressionEvaluator (Tachyon Expressions).
 */
float evaluate_expression_unified(const std::string& expr, const TextAnimatorContext& ctx) {
    if (expr.empty()) return kOne;

    using namespace tachyon::expressions;

    ExpressionContext context;
    context.time = static_cast<double>(ctx.time);
    context.seed = static_cast<std::uint64_t>(ctx.glyph_index) ^ 0x5EEDULL;
    
    // Map TextAnimator variables to ExpressionContext
    context.variables["index"] = static_cast<double>(ctx.glyph_index);
    context.variables["total_glyphs"] = static_cast<double>(ctx.total_glyphs);
    context.variables["cluster_index"] = static_cast<double>(ctx.cluster_index);
    context.variables["total_clusters"] = static_cast<double>(ctx.total_clusters);
    context.variables["word_index"] = static_cast<double>(ctx.word_index);
    context.variables["total_words"] = static_cast<double>(ctx.total_words);
    context.variables["line_index"] = static_cast<double>(ctx.line_index);
    context.variables["total_lines"] = static_cast<double>(ctx.total_lines);
    context.variables["non_space_index"] = static_cast<double>(ctx.non_space_glyph_index);
    context.variables["total_non_space_glyphs"] = static_cast<double>(ctx.total_non_space_glyphs);
    context.variables["is_rtl"] = ctx.is_rtl ? 1.0 : 0.0;
    context.variables["is_space"] = ctx.is_space ? 1.0 : 0.0;
    context.variables["t"] = context.time; // short alias for time

    EvaluationResult result = CoreExpressionEvaluator::evaluate(expr, context);
    
    if (!result.success) {
        // Fallback to 1.0 on error to avoid breaking the render
        return kOne;
    }

    return std::clamp(static_cast<float>(result.value), kZero, kOne);
}

// Export for use in text_animator_coverage.cpp
float evaluate_expression_wrapper(const std::string& expr, const TextAnimatorContext& ctx) {
    return evaluate_expression_unified(expr, ctx);
}

} // namespace tachyon::text
