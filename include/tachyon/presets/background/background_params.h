#pragma once

#include "tachyon/presets/preset_base.h"
#include <string>

namespace tachyon::presets {

struct BackgroundParams : LayerParams {
    std::string kind{"aura"}; // "aura", "grid", "stars", "noise", "minimal_white", etc.
    std::string palette{"neon_night"}; // "neon_night", "dark_tech", "premium_dark", etc.
    uint64_t seed{0};
    float speed{1.0f};
};

} // namespace tachyon::presets
