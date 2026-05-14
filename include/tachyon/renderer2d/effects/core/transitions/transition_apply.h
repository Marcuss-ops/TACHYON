#pragma once

#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <string>

namespace tachyon {

class TransitionRegistry;

/**
 * @brief Input parameters for applying a transition.
 */
struct TransitionApplyRequest {
    std::string transition_id;
    const renderer2d::SurfaceRGBA* from{nullptr};
    const renderer2d::SurfaceRGBA* to{nullptr};
    float progress{0.0f};
    int thread_count{1};
};

/**
 * @brief Result of a transition application.
 */
struct TransitionApplyResult {
    bool ok{false};
    renderer2d::SurfaceRGBA output;
    std::string error_message;
};

/**
 * @brief High-level application of a transition between two surfaces.
 * 
 * If 'to' is null, it is treated as a transparent surface of the same size as 'from'.
 */
TACHYON_API TransitionApplyResult apply_transition(
    const TransitionApplyRequest& request,
    const TransitionRegistry& registry);

} // namespace tachyon
