#pragma once

#include "tachyon/core/types/color_spec.h"
#include "tachyon/core/math/vector2.h"
#include "tachyon/core/math/vector3.h"
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace tachyon::registry {

enum class ParameterType {
    Float,
    Double,
    Int,
    Bool,
    String,
    Color,
    Vector2,
    Vector3,
    Enum,
    AssetPath,
    Easing,
};

using ParameterValue = std::variant<
    float,
    double,
    int,
    bool,
    std::string,
    ColorSpec,
    math::Vector2,
    math::Vector3
>;

struct EnumOption {
    std::string id;
    std::string label;
};

struct ParameterDef {
    std::string id;
    std::string label;
    std::string description;
    ParameterType type{ParameterType::String};
    ParameterValue default_value{std::string{}};
    std::optional<double> min_value;
    std::optional<double> max_value;
    std::vector<EnumOption> enum_options;
    bool animatable{true};
    bool required{false};

    ParameterDef(std::string id, std::string label, std::string description, ParameterValue def, std::optional<double> min = std::nullopt, std::optional<double> max = std::nullopt)
        : id(std::move(id)), label(std::move(label)), description(std::move(description)), default_value(std::move(def)), min_value(min), max_value(max) {
        
        // Infer type from variant
        if (std::holds_alternative<float>(default_value)) type = ParameterType::Float;
        else if (std::holds_alternative<double>(default_value)) type = ParameterType::Double;
        else if (std::holds_alternative<int>(default_value)) type = ParameterType::Int;
        else if (std::holds_alternative<bool>(default_value)) type = ParameterType::Bool;
        else if (std::holds_alternative<std::string>(default_value)) type = ParameterType::String;
        else if (std::holds_alternative<ColorSpec>(default_value)) type = ParameterType::Color;
        else if (std::holds_alternative<math::Vector2>(default_value)) type = ParameterType::Vector2;
        else if (std::holds_alternative<math::Vector3>(default_value)) type = ParameterType::Vector3;
    }

    ParameterDef(std::string id, std::string label, std::string description, ParameterType type, ParameterValue def, std::optional<double> min = std::nullopt, std::optional<double> max = std::nullopt)
        : id(std::move(id)), label(std::move(label)), description(std::move(description)), type(type), default_value(std::move(def)), min_value(min), max_value(max) {}

    ParameterDef() = default;
};

struct ParameterSchema {
    std::vector<ParameterDef> params;

    ParameterSchema(std::initializer_list<ParameterDef> list) : params(list) {}
    ParameterSchema() = default;
};

} // namespace tachyon::registry
