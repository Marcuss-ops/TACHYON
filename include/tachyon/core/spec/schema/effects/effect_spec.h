#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"
#include <string>
#include <map>
#include <vector>

namespace tachyon {

/**
 * @brief Evaluation result of an effect at a specific point in time.
 * Contains flat, static values resolved from keyframes or expressions.
 */
struct EvaluatedEffect {
    bool enabled{true};
    std::string type;
    std::map<std::string, std::variant<double, bool, std::string, ColorSpec, math::Vector2>> params;
};

/**
 * @brief Unified effect specification supporting both static and animated parameters.
 */
struct EffectSpec {
    bool enabled{true};
    std::string type;
    
    std::map<std::string, ParameterValue> params;

    TACHYON_API void set_param(const std::string& key, ParameterValue value);

    [[nodiscard]] TACHYON_API EvaluatedEffect evaluate(double time_seconds) const;
};

} // namespace tachyon
