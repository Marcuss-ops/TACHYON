#pragma once

#include "tachyon/core/expressions/expression_engine.h"

// Forward expression evaluation functionality to renderer2d namespace
namespace tachyon::renderer2d::expressions {

using ExpressionContext = tachyon::expressions::ExpressionContext;
using EvaluationResult = tachyon::expressions::EvaluationResult;

inline void bind_standard_expression_variables(ExpressionContext& context) {
    tachyon::expressions::bind_standard_expression_variables(context);
}

class Renderer2DExpressionEvaluator {
public:
    static EvaluationResult evaluate(const std::string& expression, const ExpressionContext& context) {
        // Use the CoreExpressionEvaluator for evaluation
        return tachyon::expressions::CoreExpressionEvaluator::evaluate(expression, context);
    }
};

} // namespace tachyon::renderer2d::expressions