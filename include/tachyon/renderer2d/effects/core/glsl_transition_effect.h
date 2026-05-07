#pragma once

#include "tachyon/renderer2d/effects/core/effect_host.h"
#include "tachyon/transition_registry.h"

namespace tachyon::renderer2d {

/**
 * Initialize built-in transitions in the global TransitionRegistry.
 * Should be called once at startup.
 */
void init_builtin_transitions(TransitionRegistry& registry);

}  // namespace tachyon::renderer2d

