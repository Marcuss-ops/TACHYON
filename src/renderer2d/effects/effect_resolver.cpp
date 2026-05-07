#include "tachyon/renderer2d/effects/effect_resolver.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include <string>

namespace tachyon::renderer2d {

ResolvedEffect resolve_effect(const EffectSpec& spec) {
    ResolvedEffect result;
    result.spec = spec;
    
    // Handle disabled effects
    if (!spec.enabled || spec.type.empty()) {
        result.valid = false;
        result.error_message = "Effect is disabled or has no type.";
        return result;
    }
    
    result.id = spec.type;
    
    // Look up in EffectRegistry (canonical source of truth)
    const auto* descriptor = EffectRegistry::instance().find(spec.type);
    
    if (!descriptor) {
        result.valid = false;
        result.error_message = "Effect '" + spec.type + "' is not registered in the registry.";
        return result;
    }
    
    result.descriptor = descriptor;
    result.valid = true;
    return result;
}

ResolvedEffect resolve_effect_by_id(const std::string& id) {
    EffectSpec spec;
    spec.enabled = true;
    spec.type = id;
    return resolve_effect(spec);
}

} // namespace tachyon::renderer2d
