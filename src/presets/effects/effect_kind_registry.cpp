#include "tachyon/presets/effects/effect_kind_registry.h"
#include "tachyon/presets/effects/effect_manifest.h"

namespace tachyon::presets {

EffectKindRegistry::EffectKindRegistry(const EffectManifest& manifest) {
    load_from_manifest(manifest);
}

void EffectKindRegistry::load_from_manifest(const EffectManifest& manifest) {
    auto specs = manifest.generate_kind_specs();
    for (auto& spec : specs) {
        register_spec(std::move(spec));
    }
}

} // namespace tachyon::presets
