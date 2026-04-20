#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace tachyon {
namespace expressions {

/**
 * Context for expression evaluation, providing variable resolution (e.g., 't').
 */
struct ExpressionContext {
    std::unordered_map<std::string, double> variables;
    std::uint64_t seed{0};
};

/**
 * Result of an expression evaluation.
 */
struct EvaluationResult {
    double value{0.0};
    bool success{true};
    std::string error;
};

/**
 * A lightweight math expression evaluator for Milestone 1.
 * Supports: +, -, *, /, ^, (), sin, cos, abs, clamp, and variable resolution.
 */
class ExpressionEvaluator {
public:
    static EvaluationResult evaluate(const std::string& expression, const ExpressionContext& context);
};

} // namespace expressions
} // namespace tachyon
