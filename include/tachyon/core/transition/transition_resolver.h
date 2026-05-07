#pragma once

#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <string>

namespace tachyon {

struct ResolvedTransition {
    std::string id;
    TransitionRuntimeKind backend{TransitionRuntimeKind::StateOnly};
    const TransitionDescriptor* descriptor{nullptr};
    LayerTransitionSpec layer_spec;
    CpuTransitionFn cpu_function{nullptr};   ///< For CPU transitions
    GlslTransitionFn glsl_function{nullptr}; ///< For GLSL transitions
    bool valid{false};
    std::string error_message;
};

ResolvedTransition resolve_transition_spec(const LayerTransitionSpec& spec);
ResolvedTransition resolve_transition_by_id(const std::string& id);

} // namespace tachyon
