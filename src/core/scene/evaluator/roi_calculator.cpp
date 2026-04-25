#include "tachyon/core/scene/state/evaluated_state.h"
#include <algorithm>

namespace tachyon::scene {

// Simple Rect struct for ROI calculations
struct Rect {
    float x1, y1, x2, y2;
};

/**
 * @brief Computes the Region of Interest (ROI) by comparing two composition states.
 */
Rect compute_roi(
    const EvaluatedCompositionState& prev,
    const EvaluatedCompositionState& curr) 
{
    if (prev.layers.size() != curr.layers.size()) {
        return {0.0f, 0.0f, 1.0f, 1.0f};
    }

    float xmin = 1.0f, ymin = 1.0f, xmax = 0.0f, ymax = 0.0f;
    bool any_change = false;

    for (std::size_t i = 0; i < curr.layers.size(); ++i) {
        const auto& l_prev = prev.layers[i];
        const auto& l_curr = curr.layers[i];

        // Check for changes (simplified: position, scale, opacity, visibility)
        bool changed = (l_prev.world_position3.x != l_curr.world_position3.x ||
                        l_prev.world_position3.y != l_curr.world_position3.y ||
                        l_prev.opacity != l_curr.opacity ||
                        l_prev.visible != l_curr.visible);

        if (changed) {
            any_change = true;
            // Bounding box of the layer (simplified: based on center and size)
            float half_w = static_cast<float>(l_curr.width) * 0.5f / static_cast<float>(curr.width);
            float half_h = static_cast<float>(l_curr.height) * 0.5f / static_cast<float>(curr.height);
            float cx = l_curr.world_position3.x / static_cast<float>(curr.width) + 0.5f;
            float cy = l_curr.world_position3.y / static_cast<float>(curr.height) + 0.5f;

            xmin = std::min(xmin, cx - half_w);
            ymin = std::min(ymin, cy - half_h);
            xmax = std::max(xmax, cx + half_w);
            ymax = std::max(ymax, cy + half_h);
        }
    }

    if (!any_change) return {0,0,0,0}; // Nothing changed

    // Clamp and return
    return {
        std::max(0.0f, xmin),
        std::max(0.0f, ymin),
        std::min(1.0f, xmax - xmin),
        std::min(1.0f, ymax - ymin)
    };
}

} // namespace tachyon::scene
