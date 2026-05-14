#pragma once

#include "tachyon/presets/preset_base.h"
#include "tachyon/core/animation/easing.h"
#include "tachyon/core/spec/schema/common/common_spec.h"
#include <map>
#include <optional>
#include <string>

namespace tachyon::presets {

// Transition ids are caller-defined; the preset registry is intentionally empty by default.

struct TransitionParams : LayerParams {
    std::string id;
    std::optional<double> duration;
    std::optional<animation::EasingPreset> easing;
    double      delay{0.0};
    Direction direction{Direction::None};
    std::map<std::string, double> parameters;
};

} // namespace tachyon::presets
