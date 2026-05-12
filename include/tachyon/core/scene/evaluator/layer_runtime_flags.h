#pragma once

#include <cstdint>

namespace tachyon::scene {

/**
 * @brief Lightweight flags for layer runtime state (visibility, type, etc.)
 */
struct LayerRuntimeFlags {
    bool visible : 1;
    bool enabled : 1;
    bool is_adjustment_layer : 1;
    bool time_remap_enabled : 1;

    [[nodiscard]] bool is_rendered() const noexcept {
        return visible && enabled;
    }
};

} // namespace tachyon::scene
