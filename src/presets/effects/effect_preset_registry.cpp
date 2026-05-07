#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::presets {

EffectSpec EffectPresetRegistry::create(std::string_view id, const registry::ParameterBag& params) const {
    if (const auto* spec = find(id)) {
        return spec->factory(params);
    }

    // Default to a no-op effect if not found
    EffectSpec effect;
    effect.type = std::string(id);
    effect.enabled = false;
    return effect;
}

} // namespace tachyon::presets
