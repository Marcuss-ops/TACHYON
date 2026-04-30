#pragma once

#include "tachyon/core/expressions/expression_engine.h"
#include "tachyon/core/properties/property.h"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace tachyon {
namespace properties {

namespace detail {
inline std::uint64_t stable_string_hash(const std::string& text) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (unsigned char ch : text) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= 1099511628211ULL;
    }
    return hash;
}

inline std::uint64_t hash_combine(std::uint64_t lhs, std::uint64_t rhs) {
    lhs ^= rhs + 0x9E3779B97F4A7C15ULL + (lhs << 6) + (lhs >> 2);
    return lhs;
}
} // namespace detail

/**
 * A property driven by a math expression string.
 */
template <typename T>
class ExpressionProperty {
public:
    explicit ExpressionProperty(std::string name, std::string expression)
        : m_name(std::move(name))
        , m_expression(std::move(expression)) {
        m_compilation = expressions::CoreExpressionEvaluator::compile(m_expression);
    }

    T sample(const PropertyEvaluationContext& context) const {
        if (!m_compilation.success) {
            return T{};
        }

        expressions::ExpressionContext expr_ctx;
        expr_ctx.variables["t"] = context.time;
        expr_ctx.variables["time"] = context.time;
        expr_ctx.variables["seed"] = static_cast<double>(context.seed);

        try {
            double value = expressions::ExpressionVM::execute(m_compilation.bytecode, expr_ctx);
            return static_cast<T>(value);
        } catch (...) {
            return T{};
        }
    }

    [[nodiscard]] T evaluate(const PropertyEvaluationContext& context) const {
        return sample(context);
    }

    std::uint64_t hash_identity(const PropertyEvaluationContext& context) const {
        const std::uint64_t expr_hash = detail::stable_string_hash(m_expression);
        return detail::hash_combine(expr_hash, context.seed);
    }

    const std::string& get_name() const {
        return m_name;
    }

    const std::vector<std::string>& get_dependencies() const {
        return m_compilation.bytecode.names;
    }

private:
    std::string m_name;
    std::string m_expression;
    expressions::CompilationResult m_compilation;
};

} // namespace properties
} // namespace tachyon
