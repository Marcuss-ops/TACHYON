#pragma once

#include "tachyon/core/media/media_interfaces.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include <string>

namespace tachyon::ops {

/**
 * @brief High-level operations for rendering procedural transitions.
 * Uses the BackendRegistry to resolve the appropriate renderer.
 */
class TransitionOps {
public:
    /**
     * @brief Applies a transition between two surfaces.
     * @param from Starting surface.
     * @param to Ending surface (can be null for some transitions).
     * @param transition_id ID of the transition (e.g., "crossfade", "wipe").
     * @param progress Progress from 0.0 to 1.0.
     * @return Result surface or error.
     */
    static core::MediaResult<renderer2d::SurfaceRGBA> apply(
        const renderer2d::SurfaceRGBA& from,
        const renderer2d::SurfaceRGBA* to,
        const std::string& transition_id,
        float progress
    );
};

} // namespace tachyon::ops
