#include <gtest/gtest.h>
#include "tachyon/runtime/execution/parallel/taskflow_runtime.h"
#include <atomic>
#include <vector>

namespace tachyon::runtime::test {

TEST(TaskflowRuntimeTest, ParallelForFramesExecutesAllTasks) {
    const std::size_t task_count = 100;
    std::atomic<std::size_t> counter{0};
    
    TaskflowRuntime runtime(4);
    runtime.parallel_for_frames(task_count, [&](std::size_t index) {
        counter.fetch_add(1);
    });
    
    EXPECT_EQ(counter.load(), task_count);
}

TEST(TaskflowRuntimeTest, ParallelForFramesHandlesZeroTasks) {
    std::atomic<std::size_t> counter{0};
    
    TaskflowRuntime runtime(4);
    runtime.parallel_for_frames(0, [&](std::size_t index) {
        counter.fetch_add(1);
    });
    
    EXPECT_EQ(counter.load(), 0);
}

TEST(TaskflowRuntimeTest, ParallelForFramesPassesCorrectIndices) {
    const std::size_t task_count = 50;
    std::vector<bool> executed(task_count, false);
    std::mutex mutex;
    
    TaskflowRuntime runtime(4);
    runtime.parallel_for_frames(task_count, [&](std::size_t index) {
        std::lock_guard<std::mutex> lock(mutex);
        executed[index] = true;
    });
    
    for (std::size_t i = 0; i < task_count; ++i) {
        EXPECT_TRUE(executed[i]) << "Task at index " << i << " was not executed";
    }
}

TEST(TaskflowRuntimeTest, ParallelForFramesHandlesSingleThread) {
    const std::size_t task_count = 10;
    std::atomic<std::size_t> counter{0};
    
    TaskflowRuntime runtime(1);
    runtime.parallel_for_frames(task_count, [&](std::size_t index) {
        counter.fetch_add(1);
    });
    
    EXPECT_EQ(counter.load(), task_count);
}

} // namespace tachyon::runtime::test
