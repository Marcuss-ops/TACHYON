#pragma once

#include "tachyon/core/spec/schema/common/common_spec.h"
#include <cstdint>

namespace tachyon::core::presets {

enum class SfxCategory {
    TypeWriting,
    Mouse,
    Photo,
    Soosh,
    MoneySound,
};

struct SfxParams {
    SfxCategory category{SfxCategory::TypeWriting};
    int variant{-1};
    uint64_t seed{0};
    float volume{1.0f};
    double in_point{0.0};
    double out_point{-1.0};
};

} // namespace tachyon::core::presets
