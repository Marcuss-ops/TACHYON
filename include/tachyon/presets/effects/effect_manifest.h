#pragma once

#include "tachyon/presets/effects/effect_specs.h"
#include <vector>

namespace tachyon::presets {

/**
 * @brief Manifest that consolidates all effect specification generation.
 * This is the single canonical source for both effect kind specs and effect preset specs.
 * It is renderer-neutral.
 */
class EffectManifest {
public:
    /**
     * @brief Generate all effect kind specs.
     */
    std::vector<EffectKindSpec> generate_kind_specs() const;

    /**
     * @brief Generate all effect preset specs.
     */
    std::vector<EffectPresetSpec> generate_preset_specs() const;

private:
    // Categories
    std::vector<EffectKindSpec> get_blur_kinds() const;
    std::vector<EffectKindSpec> get_color_kinds() const;
    std::vector<EffectKindSpec> get_transition_kinds() const;
    std::vector<EffectKindSpec> get_distortion_kinds() const;
    std::vector<EffectKindSpec> get_generator_kinds() const;
    std::vector<EffectKindSpec> get_stylize_kinds() const;

    std::vector<EffectPresetSpec> get_blur_presets() const;
    std::vector<EffectPresetSpec> get_color_presets() const;
    std::vector<EffectPresetSpec> get_transition_presets() const;
    std::vector<EffectPresetSpec> get_distortion_presets() const;
    std::vector<EffectPresetSpec> get_generator_presets() const;
    std::vector<EffectPresetSpec> get_stylize_presets() const;
};

} // namespace tachyon::presets
