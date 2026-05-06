#include "tachyon/runtime/policy/surface_pool_policy.h"

#include <algorithm>

namespace tachyon::runtime {

std::size_t SurfacePoolPolicy::resolve(
    std::uint32_t width,
    std::uint32_t height,
    std::size_t worker_count) const {

    const bool large_frame = (width >= 3840 || height >= 2160);

    // Basic heuristic: workers + buffer
    std::size_t desired = worker_count + extra_surfaces;

    // For very large frames, cap the pool to avoid excessive memory usage
    if (large_frame) {
        desired = std::min(desired, std::size_t{8});
    }

    desired = std::max(desired, min_surfaces);
    desired = std::min(desired, max_surfaces);

    return desired;
}

} // namespace tachyon::runtime
