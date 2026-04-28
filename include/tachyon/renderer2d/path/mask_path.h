#pragma once

#include "tachyon/core/math/algebra/vector2.h"
#include <vector>

namespace tachyon::renderer2d {

/**
 * @brief A single vertex in a bezier mask path.
 */
struct MaskVertex {
    math::Vector2 position;
    math::Vector2 in_tangent;  ///< Relative to position.
    math::Vector2 out_tangent; ///< Relative to position.
    
    // Per-vertex variable feather
    float feather_inner{0.0f}; ///< Inner feather expansion (negative).
    float feather_outer{0.0f}; ///< Outer feather expansion (positive).
};

/**
 * @brief A closed or open bezier path used for masking and roto.
 */
struct MaskPath {
    std::vector<MaskVertex> vertices;
    bool is_closed{true};
    bool is_inverted{false};
};

} // namespace tachyon::renderer2d

