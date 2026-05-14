#include "tachyon/renderer2d/effects/core/transitions/transition_fast_paths.h"
#include <array>
#include <string_view>

namespace tachyon::renderer2d {

using FastPathKernelFn = void(*)(SurfaceRGBA&, const SurfaceRGBA&, const SurfaceRGBA*, float, int);

struct FastPathRegistryEntry {
    std::string_view id;
    FastPathKernelFn kernel;
};

static constexpr std::array<FastPathRegistryEntry, 14> kFastPathRegistry = {{
    { "soft_zoom_blur", apply_soft_zoom_blur_fused_direct },
    { "crossfade", apply_crossfade_fused_direct },
    { "slide", apply_slide_fused_direct },
    { "push_left", apply_slide_fused_direct },
    { "swipe_left", apply_slide_fused_direct },
    { "wipe_linear", apply_wipe_linear_fused_direct },
    { "linear_wipe", apply_wipe_linear_fused_direct },
    { "smooth_wipe", apply_smooth_wipe_fused_direct },
    { "wipe_soft", apply_smooth_wipe_fused_direct },
    { "circle_iris", apply_circle_iris_fused_direct },
    { "iris_circle", apply_circle_iris_fused_direct },
    { "iris", apply_circle_iris_fused_direct },
    { "flash_cut", apply_flash_cut_fused_direct },
    { "flash", apply_flash_cut_fused_direct }
}};

static_assert(kFastPathRegistry.size() == 14, "Update kFastPathRegistry size");

consteval bool has_fast_path_registry_duplicates() {
    for (size_t i = 0; i < kFastPathRegistry.size(); ++i) {
        for (size_t j = i + 1; j < kFastPathRegistry.size(); ++j) {
            if (kFastPathRegistry[i].id == kFastPathRegistry[j].id) return true;
        }
    }
    return false;
}

static_assert(!has_fast_path_registry_duplicates(), "FATAL: Duplicate transition identifier detected within the Fast Path Registry.");

bool apply_transition_fast_path(
    const std::string& transition_id,
    SurfaceRGBA& output,
    const SurfaceRGBA& from,
    const SurfaceRGBA* to,
    float progress,
    int thread_count) {
    
    // Dimension check: Fast-paths require matching dimensions
    if (output.width() != from.width() || output.height() != from.height()) {
        return false;
    }
    if (to && (output.width() != to->width() || output.height() != to->height())) {
        return false;
    }

    // Normalize ID by stripping tachyon.transition. prefix if present
    std::string_view tid = transition_id;
    const std::string_view prefix = "tachyon.transition.";
    if (tid.starts_with(prefix)) {
        tid.remove_prefix(prefix.length());
    }

    for (const auto& entry : kFastPathRegistry) {
        if (entry.id == tid) {
            entry.kernel(output, from, to, progress, thread_count);
            return true;
        }
    }

    return false;
}

} // namespace tachyon::renderer2d
