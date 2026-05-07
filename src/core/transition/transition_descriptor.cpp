#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/policy/engine_policy.h"

namespace tachyon {

TransitionResolutionResult resolve_transition(const std::string& id, const TransitionRegistry& registry) {
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

} // namespace tachyon
