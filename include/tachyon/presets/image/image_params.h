#pragma once

#include "tachyon/presets/preset_base.h"
#include <string>

namespace tachyon::presets {

struct ImageParams : LayerParams {
    std::string path;
    std::string fit{"cover"};           // "cover", "contain", "fill", "none"
    std::string animation{"none"};      // "none", "slow_zoom", "pan_left", "pan_right",
                                        // "pan_up", "pan_down", "ken_burns"
    float zoom_from{1.0f};
    float zoom_to{1.08f};
    float pan_distance{0.05f};          // fraction of width/height
};

} // namespace tachyon::presets
