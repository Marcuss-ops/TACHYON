#include "tachyon/renderer2d/path/flattening/path_flattening.h"
#include <algorithm>
#include <cmath>

namespace tachyon::renderer2d {

void PathFlattener::flatten_cubic(
    const math::Vector2& p0,
    const math::Vector2& p1,
    const math::Vector2& p2,
    const math::Vector2& p3,
    std::vector<math::Vector2>& out,
    float tolerance,
    std::uint32_t depth) {

    const math::Vector2 chord = p3 - p0;
    const float chord_length = chord.length();
    const float distance_1 = chord_length > 0.0f ? std::abs((p1.x - p0.x) * chord.y - (p1.y - p0.y) * chord.x) / chord_length : (p1 - p0).length();
    const float distance_2 = chord_length > 0.0f ? std::abs((p2.x - p0.x) * chord.y - (p2.y - p0.y) * chord.x) / chord_length : (p2 - p0).length();

    if ((std::max(distance_1, distance_2) <= tolerance) || depth >= 12U) {
        out.push_back(p3);
        return;
    }

    const math::Vector2 p01 = (p0 + p1) * 0.5f;
    const math::Vector2 p12 = (p1 + p2) * 0.5f;
    const math::Vector2 p23 = (p2 + p3) * 0.5f;
    const math::Vector2 p012 = (p01 + p12) * 0.5f;
    const math::Vector2 p123 = (p12 + p23) * 0.5f;
    const math::Vector2 p0123 = (p012 + p123) * 0.5f;

    flatten_cubic(p0, p01, p012, p0123, out, tolerance, depth + 1U);
    flatten_cubic(p0123, p123, p23, p3, out, tolerance, depth + 1U);
}

} // namespace tachyon::renderer2d
