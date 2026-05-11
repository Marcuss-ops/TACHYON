#include "tachyon/runtime/execution/parallel/taskflow_runtime.h"

#if defined(TACHYON_ENABLE_TASKFLOW)
#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>
#endif

#include <algorithm>
#include <thread>

namespace tachyon::runtime {

TaskflowRuntime::TaskflowRuntime(std::size_t worker_count)
    : worker_count_(std::max<std::size_t>(1, worker_count)) {}

void TaskflowRuntime::parallel_for_frames(
    std::size_t task_count,
    const std::function<void(std::size_t index)>& fn
) {
    if (task_count == 0) {
        return;
    }

#if defined(TACHYON_ENABLE_TASKFLOW)
    tf::Executor executor(worker_count_);
    tf::Taskflow taskflow;

    taskflow.for_each_index(
        std::size_t{0},
        task_count,
        std::size_t{1},
        [&](std::size_t index) {
            fn(index);
        }
    );

    executor.run(taskflow).wait();
#else
    // Fallback to sequential execution if Taskflow is disabled
    for (std::size_t i = 0; i < task_count; ++i) {
        fn(i);
    }
#endif
}

} // namespace tachyon::runtime
