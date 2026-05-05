#pragma once

#include "tachyon/presets/preset_base.h"
#include "tachyon/core/animation/easing.h"
#include <map>
#include <string>

namespace tachyon::presets {

// Transition ids are caller-defined; the preset registry is intentionally empty by default.

struct TransitionParams : LayerParams {
    std::string id;
    double      duration{0.4};
    animation::EasingPreset easing{animation::EasingPreset::EaseOut};
    double      delay{0.0};
    std::string direction{"none"}; // "up", "down", "left", "right", "random"
    std::map<std::string, double> parameters;
};

} // namespace tachyon::presets
