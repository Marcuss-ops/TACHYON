#include "tachyon/core/transition/transition_resolver.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/policy/engine_policy.h"
#include <string>

namespace tachyon {

ResolvedTransition resolve_transition_spec(const LayerTransitionSpec& spec) {
    ResolvedTransition result;
    result.layer_spec = spec;
    
    // Resolve the transition id
    std::string id = spec.transition_id.empty() ? spec.type : spec.transition_id;
    if (id.empty()) {
        result.valid = false;
        result.error_message = "Transition ID is empty.";
        return result;
    }
    result.id = id;
    
    // Look up in TransitionRegistry (canonical source of truth)
    const auto* desc = TransitionRegistry::instance().resolve(id);
    
    if (!desc) {
        result.valid = false;
        result.error_message = "Transition '" + id + "' is not registered in the registry.";
        return result;
    }
    
    result.descriptor = desc;
    result.backend = desc->runtime_kind;
    
    // Set appropriate function based on backend capabilities
    if (desc->runtime_kind == TransitionRuntimeKind::CpuPixel && desc->capabilities.supports_cpu) {
        result.cpu_function = desc->cpu_fn;
    } else if (desc->runtime_kind == TransitionRuntimeKind::Glsl && desc->capabilities.supports_gpu) {
        result.glsl_function = desc->glsl_fn;
    }
    
    result.valid = true;
    return result;
}

ResolvedTransition resolve_transition_by_id(const std::string& id) {
    LayerTransitionSpec spec;
    spec.transition_id = id;
    spec.type = id;
    return resolve_transition_spec(spec);
}

} // namespace tachyon
