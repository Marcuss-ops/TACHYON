#pragma once

#include "tachyon/core/expressions/expression_engine.h"

// Forward expression evaluation functionality to renderer2d namespace
namespace tachyon::renderer2d::expressions {

using ExpressionContext = tachyon::expressions::ExpressionContext;
using EvaluationResult = tachyon::expressions::EvaluationResult;

class ExpressionEvaluator {
public:
    static EvaluationResult evaluate(const std::string& expression, const ExpressionContext& context) {
        return tachyon::expressions::ExpressionEvaluator::evaluate(expression, context);
    }
};

} // namespace tachyon::renderer2d::expressions