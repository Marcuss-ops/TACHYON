#include "tachyon/renderer2d/effects/core/transitions/transition_fast_paths.h"
#include <iostream>


namespace tachyon::renderer2d {

bool apply_transition_fast_path(
    const std::string& transition_id,
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count) {
    
    // Normalize ID by stripping tachyon.transition. prefix if present
    std::string_view tid = transition_id;
    const std::string_view prefix = "tachyon.transition.";
    if (tid.starts_with(prefix)) {
        tid.remove_prefix(prefix.length());
    }

    std::cout << "[debug] fast-path attempt for: " << tid << " (original: " << transition_id << ")" << std::endl;

    if (tid == "soft_zoom_blur") {
        apply_soft_zoom_blur_fused_direct(output, from, to, progress, thread_count);
        return true;
    }
    
    if (tid == "crossfade") {
        apply_crossfade_fused_direct(output, from, to, progress, thread_count);
        return true;
    }

    if (tid == "slide" || tid == "push_left" || tid == "swipe_left") {
        apply_slide_fused_direct(output, from, to, progress, thread_count);
        return true;
    }

    if (tid == "wipe_linear" || tid == "linear_wipe") {
        apply_wipe_linear_fused_direct(output, from, to, progress, thread_count);
        return true;
    }

    if (tid == "smooth_wipe" || tid == "wipe_soft") {
        apply_smooth_wipe_fused_direct(output, from, to, progress, thread_count);
        return true;
    }

    return false;
}

} // namespace tachyon::renderer2d
