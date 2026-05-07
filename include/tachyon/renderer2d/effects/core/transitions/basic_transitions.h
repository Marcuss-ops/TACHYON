#pragma once

namespace tachyon {
class TransitionRegistry;
}

namespace tachyon::renderer2d {

/**
 * Register basic built-in transitions (Fade, Slide, Zoom, etc.)
 */
void register_basic_transitions(TransitionRegistry& registry);

} // namespace tachyon::renderer2d
