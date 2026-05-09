#include "tachyon/runtime/policy/worker_policy.h"

#include <algorithm>

namespace tachyon::runtime {

RenderWorkerBudget RenderWorkerPolicy::resolve(std::size_t hardware_threads) const {
    const std::size_t safe_hardware = (hardware_threads == 0) ? 1 : hardware_threads;

    RenderWorkerBudget budget;
    budget.total_threads = safe_hardware;

    if (requested_workers == 1) {
        budget.frame_concurrency = 1;
        budget.pixel_concurrency = safe_hardware;
    } else {
        // Budgeting Strategy: 
        // We want to avoid spawning N_frames * N_pixels threads.
        // For hardware_threads <= 4, we prioritize frame parallelism (1 thread per frame).
        // For hardware_threads > 4, we use a hybrid approach.
        
        std::size_t target_frames = (requested_workers > 0) ? requested_workers : safe_hardware;
        
        if (max_workers > 0) {
            target_frames = std::min(target_frames, max_workers);
        }
        target_frames = std::max(target_frames, min_workers);

        if (target_frames >= safe_hardware) {
            // Maximum frame parallelism, 1 thread per pixel operation
            budget.frame_concurrency = safe_hardware;
            budget.pixel_concurrency = 1;
        } else {
            // Balance: e.g. 8 cores, target 2 frames -> 4 threads per frame
            budget.frame_concurrency = target_frames;
            budget.pixel_concurrency = safe_hardware / target_frames;
        }
    }

    // Final safety check
    if (budget.frame_concurrency == 0) budget.frame_concurrency = 1;
    if (budget.pixel_concurrency == 0) budget.pixel_concurrency = 1;

    return budget;
}

} // namespace tachyon::runtime
