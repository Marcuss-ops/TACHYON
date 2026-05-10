#include "tachyon/renderer2d/effects/core/transitions/transition_fast_paths.h"


namespace tachyon::renderer2d {

bool apply_transition_fast_path(
    const std::string& transition_id,
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count) {
    
    // Check if we have a fast path for this transition
    if (transition_id == "soft_zoom_blur") {
        apply_soft_zoom_blur_fused_direct(output, from, to, progress, thread_count);
        return true;
    }
    
    // Future fast paths: fade, wipe, etc.
    // if (transition_id == "fade") { ... }

    return false;
}

} // namespace tachyon::renderer2d
