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

private:
    std::size_t worker_count_ = 1;
#if defined(TACHYON_ENABLE_TASKFLOW)
    struct Impl;
    std::unique_ptr<Impl> impl_;
#endif
};

} // namespace tachyon::runtime
