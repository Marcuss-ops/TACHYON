#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "tachyon/core/expressions/ast.h"
#include "tachyon/core/expressions/expression_vm.h"

namespace tachyon {
namespace expressions {

/**
 * Context for expression evaluation, providing variable resolution (e.g., 't').
 */
struct ExpressionContext {
    std::unordered_map<std::string, double> variables;
    std::unordered_map<std::string, std::vector<std::vector<std::string>>> tables;
    std::uint64_t seed{0};
    std::uint32_t layer_index{0};
    double value{0.0};

    // Callback for valueAtTime(property_index, time)
    std::function<double(int, double)> property_sampler;

    // Callback for data(table_id, row, col)
    // First argument is table index (as double)
    std::function<double(double, double, double)> table_lookup;
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
 * Result of expression compilation.
 */
struct CompilationResult {
    Bytecode bytecode;
    std::unique_ptr<ASTNode> ast;
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
    
    // Compiles a string into an AST and Bytecode
    static CompilationResult compile(const std::string& expression);
};

} // namespace expressions
} // namespace tachyon
