#pragma once

#include "tachyon/presets/preset_base.h"

namespace tachyon::presets {

enum class SfxCategory {
    TypeWriting,
    Mouse,
    Photo,
    Soosh,
    MoneySound,
};

struct SfxParams : LayerParams {
    SfxCategory category{SfxCategory::TypeWriting};
    int variant{-1};
    uint64_t seed{0};
    float volume{1.0f};
};

} // namespace tachyon::presets
