#pragma once

#include "tachyon/core/math/vector3.h"
#include <vector>
#include <cstddef>

namespace tachyon {

enum class CullTest {
    Inside,
    Outside,
    Intersecting
};

struct BoundingBox3D {
    math::Vector3 min{math::Vector3::zero()};
    math::Vector3 max{math::Vector3::zero()};

    math::Vector3 center() const { return (min + max) * 0.5f; }
    math::Vector3 extents() const { return max - min; }
    bool is_empty() const { return min.x >= max.x || min.y >= max.y || min.z >= max.z; }
};

struct CullingResult {
    std::vector<std::size_t> visible_indices;
    std::vector<std::size_t> culled_indices;
    std::size_t total_objects{0};
    std::size_t visible_count{0};
    std::size_t culled_count{0};
    double culling_time_ms{0.0};
};

} // namespace tachyon
