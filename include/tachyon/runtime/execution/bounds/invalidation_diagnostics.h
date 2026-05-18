#pragma once

#include "tachyon/renderer2d/geometry/dirty_region.h"
#include <cstdint>
#include <vector>

namespace tachyon {

struct LayerFrameBounds {
    std::uint64_t layer_id{0};
    renderer2d::IntRect previous_bounds;
    renderer2d::IntRect current_bounds;
    renderer2d::IntRect dirty_bounds;
    bool full_frame{false};
};

struct FrameInvalidationDiagnostics {
    renderer2d::DirtyRegion dirty_region;
    std::vector<LayerFrameBounds> layer_bounds;
    bool full_frame_invalidation{false};

    [[nodiscard]] double compute_dirty_area_ratio(int frame_width, int frame_height) const {
        if (frame_width <= 0 || frame_height <= 0) return 0.0;
        
        const auto bounds = dirty_region.bounds();
        if (bounds.empty()) return 0.0;

        const double dirty_area = static_cast<double>(bounds.width) * static_cast<double>(bounds.height);
        const double total_area = static_cast<double>(frame_width) * static_cast<double>(frame_height);
        
        return dirty_area / total_area;
    }
};

} // namespace tachyon