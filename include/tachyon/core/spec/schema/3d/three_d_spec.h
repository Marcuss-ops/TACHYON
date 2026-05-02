#pragma once

#include "tachyon/core/spec/schema/properties/property_spec.h"
#include <string>
#include <vector>
#include <map>

namespace tachyon {

struct ThreeDModifierSpec {
    std::string type;
    std::map<std::string, AnimatedScalarSpec> scalar_params;
    std::map<std::string, AnimatedVector3Spec> vector3_params;
    std::map<std::string, std::string> string_params;
};

struct ThreeDSpec {
    bool enabled{false};
    std::vector<ThreeDModifierSpec> modifiers;
};

} // namespace tachyon
