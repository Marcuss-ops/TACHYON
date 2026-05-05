#pragma once

#include "tachyon/core/types/color_spec.h"
#include "tachyon/presets/preset_base.h"
#include <string>
#include <cstdint>

namespace tachyon::presets {

struct BackgroundParams : LayerParams {
    std::string kind{};
    std::string palette{"neon_night"}; // "neon_night", "dark_tech", "premium_dark", etc.
    uint64_t seed{0};
    float speed{1.0f};
    
    // Optional overrides for procedural presets
    std::optional<float> frequency;
    std::optional<float> amplitude;
    std::optional<float> scale;
    std::optional<float> contrast;
    std::optional<float> softness;
    std::optional<float> grain_amount;
    std::optional<std::string> shape;
    std::optional<float> spacing;
    std::optional<float> border_width;
    
    std::optional<ColorSpec> color_a;
    std::optional<ColorSpec> color_b;
    std::optional<ColorSpec> color_c;

    float mouse_x{0.5f};
    float mouse_y{0.5f};
    std::optional<float> mouse_influence;
    std::optional<float> mouse_radius;
};

} // namespace tachyon::presets
