#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"

namespace tachyon {

void register_builtin_transitions(TransitionRegistry& reg) {
    renderer2d::register_basic_transitions(reg);
    renderer2d::register_artistic_transitions(reg);
    renderer2d::register_light_leak_implementations(reg);
}

} // namespace tachyon
