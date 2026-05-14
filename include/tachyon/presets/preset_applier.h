#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/registry/parameter_bag.h"
#include <string>

namespace tachyon::presets {

/**
 * @brief External adapter to apply presets to core specs.
 * This decouples the core builders from the preset registries.
 */
class PresetApplier {
public:
    /**
     * @brief Applies a 2D animation preset to a layer.
     */
    static void apply_animation2d(LayerSpec& layer, const std::string& preset_id, const registry::ParameterBag& params = {});

    /**
     * @brief Adds an effect to a layer from a preset.
     */
    static void apply_effect(LayerSpec& layer, const std::string& preset_id, const registry::ParameterBag& params = {});

    /**
     * @brief Applies a text animation preset to a layer.
     */
    static void apply_text_animation(LayerSpec& layer, const std::string& preset_id, const registry::ParameterBag& params = {});

    /**
     * @brief Sets a composition background from a preset.
     */
    static void apply_background(CompositionSpec& comp, const std::string& preset_id, const registry::ParameterBag& params = {});
};

} // namespace tachyon::presets
