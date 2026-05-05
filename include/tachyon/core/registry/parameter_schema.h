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
    ParameterType type{ParameterType::String};
    ParameterValue default_value{std::string{}};
    std::optional<double> min_value;
    std::optional<double> max_value;
    std::vector<EnumOption> enum_options;
    bool animatable{true};
    bool required{false};
};

struct ParameterSchema {
    std::vector<ParameterDef> params;
};

} // namespace tachyon::registry
