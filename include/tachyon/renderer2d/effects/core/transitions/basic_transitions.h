#pragma once

namespace tachyon {
class TransitionRegistry;
struct TransitionDescriptor;
}

namespace tachyon::renderer2d {

/**
 * Attaches CPU implementation functions to basic transition descriptors.
 */
void resolve_basic_transition_implementations(tachyon::TransitionDescriptor& descriptor);

} // namespace tachyon::renderer2d
