#pragma once

#include "tachyon/core/math/vector3.h"
#include <vector>

namespace tachyon::properties {

/**
 * @brief A keyframe for 3D spatial paths (position, POI, anchor point).
 */
struct SpatialKeyframe {
    float time{0.0f};
    math::Vector3 position{0.0f, 0.0f, 0.0f};
    math::Vector3 in_tangent{0.0f, 0.0f, 0.0f};  ///< Relative to position
    math::Vector3 out_tangent{0.0f, 0.0f, 0.0f}; ///< Relative to position
    bool linear{false};                         ///< If true, use linear segment, skip tangents
};

/**
 * @brief Interpolator for 3D spatial paths.
 * 
 * Unlike scalar interpolation, spatial interpolation uses cubic-hermite splines
 * in 3D space. This allows for smooth "curved" paths in the viewport.
 */
class SpatialInterpolator {
public:
    /**
     * @brief Evaluate a 3D position at time t.
     */
    [[nodiscard]] static math::Vector3 evaluate(
        const std::vector<SpatialKeyframe>& keyframes,
        float t);
    
    /**
     * @brief Generate a list of points representing the 3D path for visualization.
     */
    [[nodiscard]] static std::vector<math::Vector3> get_path(
        const std::vector<SpatialKeyframe>& keyframes,
        int segments_per_segment = 20);

private:
    [[nodiscard]] static std::size_t find_segment(
        const std::vector<SpatialKeyframe>& keyframes,
        float t);
};

} // namespace tachyon::properties
