#pragma once

namespace tachyon {
class TransitionRegistry;
}

namespace tachyon::renderer2d {

/**
 * Register artistic and creative built-in transitions (Kaleidoscope, Ripple, Glitch, etc.)
 */
void register_artistic_transitions(TransitionRegistry& registry);

} // namespace tachyon::renderer2d
