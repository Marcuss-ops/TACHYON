#pragma once

#include <cstddef>
#include <cstdint>

namespace tachyon::runtime {

/**
 * @brief Policy for resolving the number of surfaces to allocate in the pool.
 * 
 * Balances concurrency needs against memory pressure, especially for high-resolution frames.
 */
struct SurfacePoolPolicy {
    std::size_t min_surfaces{2};
    std::size_t max_surfaces{16};
    std::size_t extra_surfaces{2}; // buffer for precomps/output

    /**
     * @brief Resolves the final surface count.
     */
    std::size_t resolve(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t worker_count) const;
};

} // namespace tachyon::runtime
