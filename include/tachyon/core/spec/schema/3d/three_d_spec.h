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
    double extrusion_depth{0.0};
    double bevel_size{0.0};
    double hole_bevel_ratio{0.0};
    std::vector<ThreeDModifierSpec> modifiers;
};

} // namespace tachyon
