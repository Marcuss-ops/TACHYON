#include "tachyon/core/scene/state/evaluated_state.h"
#include "tachyon/core/math/rect.h"
#include <algorithm>

namespace tachyon::scene {

/**
 * @brief Computes the Region of Interest (ROI) by comparing two composition states.
 */
math::RectF compute_roi(
    const EvaluatedCompositionState& prev,
    const EvaluatedCompositionState& curr) 
{
    if (prev.layers.size() != curr.layers.size()) {
        return math::RectF::unit();
    }

    float xmin = 1.0f, ymin = 1.0f, xmax = 0.0f, ymax = 0.0f;
    bool any_change = false;

    for (std::size_t i = 0; i < curr.layers.size(); ++i) {
        const auto& l_prev = prev.layers[i];
        const auto& l_curr = curr.layers[i];

        // Check for changes (simplified: position, scale, opacity, visibility)
        bool changed = (l_prev.transform.world_matrix.m[0][2] != l_curr.transform.world_matrix.m[0][2] ||
                        l_prev.transform.world_matrix.m[1][2] != l_curr.transform.world_matrix.m[1][2] ||
                        l_prev.transform.opacity != l_curr.transform.opacity ||
                        l_prev.identity.visible != l_curr.identity.visible ||
                        l_prev.transform.local_transform.position.x != l_curr.transform.local_transform.position.x); 

        if (changed) {
            any_change = true;
            // Bounding box of the layer (simplified: based on center and size)
            float half_w = static_cast<float>(l_curr.transform.width) * 0.5f / static_cast<float>(curr.width);
            float half_h = static_cast<float>(l_curr.transform.height) * 0.5f / static_cast<float>(curr.height);
            float cx = l_curr.transform.world_matrix.m[0][2] / static_cast<float>(curr.width) + 0.5f;
            float cy = l_curr.transform.world_matrix.m[1][2] / static_cast<float>(curr.height) + 0.5f;

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
