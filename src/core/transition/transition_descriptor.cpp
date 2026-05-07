#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/policy/engine_policy.h"

namespace tachyon {

TransitionResolutionResult resolve_transition(const std::string& id, TransitionRegistry& registry) {
    TransitionResolutionResult result;
    
    const auto* desc = registry.resolve(id);
    if (!desc) {
        result.status = TransitionResolutionResult::Status::UnknownTransition;
        result.error_message = "Transition '" + id + "' is not registered in the registry.";
        return result;
    }

    result.status = TransitionResolutionResult::Status::Ok;
    result.backend = desc->runtime_kind;

    if (desc->runtime_kind == TransitionRuntimeKind::CpuPixel && desc->capabilities.supports_cpu) {
        result.cpu_function = desc->cpu_fn;
    } else if (desc->runtime_kind == TransitionRuntimeKind::Glsl && desc->capabilities.supports_gpu) {
        result.glsl_function = desc->glsl_fn;
    }

    return result;
}

void register_transition_descriptor(const TransitionDescriptor& desc, TransitionRegistry& registry) {
    registry.register_descriptor(desc);
}

TransitionDescriptor make_descriptor(const TransitionBuiltinSpec& spec) {
    TransitionDescriptor desc;
    desc.id = std::string(spec.id);
    desc.display_name = std::string(spec.display_name);
    desc.description = std::string(spec.description);
    desc.category = spec.category;
    desc.runtime_kind = spec.runtime_kind;
    desc.capabilities.supports_cpu = spec.supports_cpu;
    desc.capabilities.supports_gpu = spec.supports_gpu;
    desc.cpu_fn = spec.cpu_fn;
    desc.glsl_fn = spec.glsl_fn;
    desc.params = spec.params;
    
    desc.aliases.reserve(spec.aliases.size());
    for (const auto& alias : spec.aliases) {
        desc.aliases.push_back(std::string(alias));
    }
    
    return desc;
}

} // namespace tachyon
