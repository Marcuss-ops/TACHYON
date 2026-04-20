#pragma once

#include "tachyon/core/math/vector2.h"
#include <vector>

namespace tachyon::renderer2d {

class PathFlattener {
public:
    static void flatten_cubic(
        const math::Vector2& p0,
        const math::Vector2& p1,
        const math::Vector2& p2,
        const math::Vector2& p3,
        std::vector<math::Vector2>& out,
        float tolerance,
        std::uint32_t depth = 0U);
};

} // namespace tachyon::renderer2d
