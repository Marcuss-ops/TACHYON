#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/effect_manifest.h"
#include <utility>

namespace tachyon::renderer2d {

EffectRegistry::EffectRegistry() {
}

void EffectRegistry::register_spec(EffectDescriptor descriptor) {
    if (descriptor.id.empty()) {
        return;
    }
    registry_.register_spec(std::move(descriptor));
}

const EffectDescriptor* EffectRegistry::find(std::string_view id) const {
    return registry_.find(id);
}

std::vector<std::string> EffectRegistry::list_ids() const {
    return registry_.list_ids();
}

void register_builtin_effects(EffectRegistry& registry, const TransitionRegistry& transition_registry) {
    EffectManifest manifest(transition_registry);
    auto descriptors = manifest.generate_descriptors();
    for (auto& desc : descriptors) {
        registry.register_spec(std::move(desc));
    }
}

} // namespace tachyon::renderer2d
