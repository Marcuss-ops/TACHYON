#include "tachyon/presets/effects/effect_preset_registry.h"
#include "tachyon/renderer2d/effects/effect_manifest.h"

namespace tachyon::presets {

EffectPresetRegistry::EffectPresetRegistry(const renderer2d::EffectManifest& manifest) {
    load_from_manifest(manifest);
}

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

void EffectPresetRegistry::load_from_manifest(const renderer2d::EffectManifest& manifest) {
    auto specs = manifest.generate_preset_specs();
    for (auto& spec : specs) {
        register_spec(std::move(spec));
    }
}

} // namespace tachyon::presets
