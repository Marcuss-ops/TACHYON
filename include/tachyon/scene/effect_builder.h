#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/renderer2d/effects/generators/light_leak_types.h"
#include "tachyon/presets/effects/effect_preset_registry.h"
#include <string_view>

namespace tachyon::scene {

using renderer2d::LightLeakPreset;

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

    EffectBuilder& light_leak(LightLeakPreset preset, float progress, float speed = 1.0f, int seed = 3);
    EffectBuilder& preset(std::string_view id, const registry::ParameterBag& params);

    LayerBuilder& done();
};

} // namespace tachyon::scene
