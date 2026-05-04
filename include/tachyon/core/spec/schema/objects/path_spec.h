#pragma once

#include "tachyon/core/math/vector2.h"
#include <vector>

namespace tachyon::spec {

/**
 * @brief Path drawing operations.
 */
enum class PathVerb {
    MoveTo,
    LineTo,
    CubicTo,
    Close
};

/**
 * @brief Path winding rules.
 */
enum class WindingRule {
    NonZero,
    EvenOdd
};

/**
 * @brief Line cap styles.
 */
enum class LineCap {
    Butt,
    Round,
    Square
};

/**
 * @brief Line join styles.
 */
enum class LineJoin {
    Miter,
    Round,
    Bevel
};

/**
 * @brief A single path command (segment).
 */
struct PathCommand {
    PathVerb verb{PathVerb::MoveTo};
    math::Vector2 p0{};
    math::Vector2 p1{};
    math::Vector2 p2{};
    
    // Per-vertex feather values
    float feather_inner{0.0f};
    float feather_outer{0.0f};
};

/**
 * @brief Complete path geometry definition.
 */
struct PathGeometry {
    std::vector<PathCommand> commands;
};

} // namespace tachyon::spec
