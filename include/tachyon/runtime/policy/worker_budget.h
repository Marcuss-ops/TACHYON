#pragma once

#include <cstddef>

namespace tachyon::runtime {

/**
 * @brief Represents the allocated thread budget for rendering operations.
 * 
 * This helps coordinate frame-level parallelism (many frames at once)
 * and pixel-level parallelism (many threads per frame, e.g. for transitions).
 */
struct RenderWorkerBudget {
    /// Number of frames to render in parallel
    std::size_t frame_concurrency{1};
    
    /// Number of threads each frame can use for internal parallelism (e.g. OpenMP)
    std::size_t pixel_concurrency{1};
    
    /// Total threads used by this budget (usually frame_concurrency * pixel_concurrency)
    std::size_t total_threads{1};
};

} // namespace tachyon::runtime
