#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/presets/effects/effect_preset_registry.h"
#include <string_view>

namespace tachyon::scene {

class LayerBuilder;

/**
 * @brief Builder for effects on a LayerSpec.
 */
class TACHYON_API EffectBuilder {
    LayerBuilder& parent_;
    const presets::EffectPresetRegistry& preset_registry_;
public:
    explicit EffectBuilder(LayerBuilder& parent, const presets::EffectPresetRegistry& preset_registry) 
        : parent_(parent), preset_registry_(preset_registry) {}

    EffectBuilder& preset(std::string_view id, const registry::ParameterBag& params);

    LayerBuilder& done();
};

} // namespace tachyon::scene
