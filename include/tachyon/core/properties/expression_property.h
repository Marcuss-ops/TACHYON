#pragma once

#include "tachyon/core/expressions/expression_engine.h"
#include "tachyon/core/properties/property.h"

#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace tachyon {
namespace properties {

/**
 * A property driven by a math expression string.
 */
template <typename T>
class ExpressionProperty : public Property<T> {
public:
    explicit ExpressionProperty(std::string name, std::string expression)
        : m_name(std::move(name))
        , m_expression(std::move(expression)) {}

    T sample(const PropertyEvaluationContext& context) const override {
        expressions::ExpressionContext expr_ctx;
        expr_ctx.variables["t"] = context.time;
        expr_ctx.variables["time"] = context.time;
        expr_ctx.variables["seed"] = static_cast<double>(context.seed);

        auto result = expressions::ExpressionEvaluator::evaluate(m_expression, expr_ctx);
        
        if (!result.success) {
            return T{};
        }

        return static_cast<T>(result.value);
    }

    [[nodiscard]] T evaluate(const PropertyEvaluationContext& context) const {
        return sample(context);
    }

    uint64_t hash_identity(const PropertyEvaluationContext& context) const override {
        return static_cast<uint64_t>(std::hash<std::string>{}(m_expression) ^ (context.seed + 0x9E3779B97F4A7C15ULL));
    }

    const std::string& get_name() const override {
        return m_name;
    }

private:
    std::string m_name;
    std::string m_expression;
};

} // namespace properties
} // namespace tachyon
