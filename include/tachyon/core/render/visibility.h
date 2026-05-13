#pragma once

#include <vector>
#include <cstddef>

namespace tachyon {

enum class CullTest {
    Inside,
    Outside,
    Intersecting
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
