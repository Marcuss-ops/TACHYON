#include "tachyon/presets/effects/effect_kind_registry.h"
#include "tachyon/presets/effects/effect_manifest.h"

namespace tachyon::presets {

EffectKindRegistry::EffectKindRegistry(const renderer2d::EffectManifest& manifest) {
    // This will be refactored to use presets::EffectManifest later
    load_from_manifest(manifest);
}

// Dummy implementation until we fully refactor renderer2d::EffectManifest
void EffectKindRegistry::load_from_manifest(const renderer2d::EffectManifest& manifest) {
    // TODO: Bridge with renderer2d::EffectManifest
}

} // namespace tachyon::presets
