#pragma once

#include <cstddef>
#include <functional>

namespace tachyon::runtime {

/**
 * @brief Wrapper around Taskflow for parallel execution of frame tasks.
 * 
 * This class provides a work-stealing scheduler for parallelizing frame-level tasks.
 * It uses a pool of worker threads to execute tasks concurrently.
 */
class TaskflowRuntime {
public:
    /**
     * @brief Construct a new TaskflowRuntime.
     * @param worker_count The number of worker threads to use.
     */
    explicit TaskflowRuntime(std::size_t worker_count);

    /**
     * @brief Parallelize a loop of frame tasks.
     * @param task_count Total number of tasks to execute.
     * @param fn Function to execute for each task index.
     */
    void parallel_for_frames(
        std::size_t task_count,
        const std::function<void(std::size_t index)>& fn
    );

private:
    std::size_t worker_count_ = 1;
};

} // namespace tachyon::runtime
