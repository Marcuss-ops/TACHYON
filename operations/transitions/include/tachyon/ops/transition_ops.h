#pragma once

#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/media/media_error.h"
#include <string>
#include <unordered_map>

namespace tachyon::ops {

/**
 * @brief Parameters for a single transition render call.
 */
struct TransitionRenderRequest {
    /// ID of the transition to apply (e.g., "crossfade")
    std::string transition_id;
    
    /// Source surface (start of transition). Mandatory.
    const renderer2d::SurfaceRGBA* from{nullptr};
    
    /// Target surface (end of transition). Optional for some transitions.
    const renderer2d::SurfaceRGBA* to{nullptr};
    
    /// Progress of the transition (0.0 to 1.0)
    float progress{0.0f};
    
    /// Number of threads to use for SIMD execution
    int thread_count{1};
};

/**
 * @brief Facade for applying procedural transitions to surfaces.
 */
class TransitionOps {
public:
    static core::MediaResult<renderer2d::SurfaceRGBA>
    render(const TransitionRenderRequest& request, const TransitionRegistry& registry);

    static core::MediaResult<renderer2d::SurfaceRGBA>
    render_builtin(const TransitionRenderRequest& request);
};

core::MediaResult<renderer2d::SurfaceRGBA>
apply_transition(
    const renderer2d::SurfaceRGBA& from,
    const renderer2d::SurfaceRGBA* to,
    std::string_view id,
    float progress
);

} // namespace tachyon::ops
