#pragma once

#include <cstdint>

namespace tachyon::scene {

/**
 * @brief Lightweight flags for layer runtime state (visibility, type, etc.)
 */
struct LayerRuntimeFlags {
    bool is_3d : 1;
    bool visible : 1;
    bool enabled : 1;
    bool is_adjustment_layer : 1;
    bool time_remap_enabled : 1;

    // Helper to check if layer should be rendered in 3D
    [[nodiscard]] bool is_rendered_3d() const noexcept {
        return is_3d && visible && enabled;
    }
};

} // namespace tachyon::scene
