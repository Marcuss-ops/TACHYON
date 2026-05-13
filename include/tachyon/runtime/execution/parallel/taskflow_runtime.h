#pragma once

#include <cstddef>
#include <functional>
#include <memory>

namespace tachyon::runtime {

class TaskflowRuntime {
public:
    explicit TaskflowRuntime(std::size_t worker_count);
    ~TaskflowRuntime();

    void parallel_for_frames(
        std::size_t task_count,
        const std::function<void(std::size_t index)>& fn
    );

    // Tile-level parallelism within a single frame.
    // Uses the same persistent executor as parallel_for_frames to avoid
    // thread creation overhead and enable future frame×tile co-scheduling.
    void parallel_for_tiles(
        std::size_t tile_count,
        const std::function<void(std::size_t index)>& fn
    );

private:
    std::size_t worker_count_ = 1;
#if defined(TACHYON_ENABLE_TASKFLOW)
    struct Impl;
    std::unique_ptr<Impl> impl_;
#endif
};

} // namespace tachyon::runtime
