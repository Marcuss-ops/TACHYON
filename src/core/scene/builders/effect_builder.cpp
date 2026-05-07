#include "tachyon/scene/effect_builder.h"
#include "tachyon/scene/builder.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::scene {

EffectBuilder& EffectBuilder::light_leak(LightLeakPreset preset, float progress, float speed, int seed) {
    registry::ParameterBag params;
    params.set("preset", static_cast<double>(static_cast<int>(preset)));
    params.set("progress", static_cast<double>(progress));
    params.set("speed", static_cast<double>(speed));
    params.set("seed", static_cast<double>(seed));
    
    return this->preset("tachyon.effect.light_leak", params);
}

EffectBuilder& EffectBuilder::preset(std::string_view id, const registry::ParameterBag& params) {
    presets::EffectPresetRegistry registry;
    parent_.spec_.effects.push_back(registry.create(id, params));
    return *this;
}

LayerBuilder& EffectBuilder::done() {
    return parent_;
}

} // namespace tachyon::scene
