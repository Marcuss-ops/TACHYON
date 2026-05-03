#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include "tachyon/core/math/vector2.h"
#include <string>
#include <variant>
#include <vector>
#include <unordered_map>

namespace tachyon {

/**
 * @brief Supported parameter types for templates.
 */
enum class ParameterType {
    String,
    Float,
    Color,
    Vector2,
    Bool
};

/**
 * @brief Value holder for parameter values.
 */
using ParameterValue = std::variant<std::string, double, ColorSpec, math::Vector2, bool>;

/**
 * @brief Definition of a template parameter.
 */
struct ParameterDefinition {
    std::string name;
    ParameterType type;
    ParameterValue default_value;
};

/**
 * @brief A binding reference used in place of a literal value.
 */
struct PropertyBinding {
    std::string parameter_name;
    bool active{false};

    [[nodiscard]] bool is_bound() const { return active; }
};

/**
 * @brief Template metadata and parameter schema.
 */
struct TemplateSchema {
    std::string template_id;
    std::vector<ParameterDefinition> parameters;
};

} // namespace tachyon
