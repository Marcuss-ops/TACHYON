#pragma once

#include "tachyon/presets/preset_base.h"
#include <string>

namespace tachyon::presets {

struct ShapeParams : LayerParams {
    std::string type{"rect"};          // "rect", "circle", "ellipse", "line"
    std::string fill_color{"#ffffff"};
    std::string stroke_color{"#000000"};
    float stroke_width{0.0f};
    float corner_radius{0.0f};        // for rect
};

} // namespace tachyon::presets
