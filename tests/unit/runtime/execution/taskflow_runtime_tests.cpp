#include "tachyon/runtime/execution/parallel/taskflow_runtime.h"
#include <atomic>
#include <vector>
#include <iostream>
#include <mutex>

namespace {

int g_failures = 0;

template<typename T>
void check_equals(T actual, T expected, const char* message) {
    if (actual != expected) {
        ++g_failures;
        std::cerr << "FAIL: " << message << " (expected " << expected << ", got " << actual << ")\n";
    }
}

} // namespace

bool run_taskflow_runtime_tests() {
    g_failures = 0;

    // Test: ParallelForFramesExecutesAllTasks
    {
        const std::size_t task_count = 100;
        std::atomic<std::size_t> counter{0};

        tachyon::runtime::TaskflowRuntime runtime(4);
        runtime.parallel_for_frames(task_count, [&](std::size_t /*index*/) {
            counter.fetch_add(1);
        });

        check_equals(counter.load(), task_count, "All tasks should be executed");
    }

    // Test: ParallelForFramesHandlesZeroTasks
    {
        std::atomic<std::size_t> counter{0};

        tachyon::runtime::TaskflowRuntime runtime(4);
        runtime.parallel_for_frames(0, [&](std::size_t /*index*/) {
            counter.fetch_add(1);
        });

        check_equals(counter.load(), 0UL, "Zero tasks should execute zero times");
    }

    // Test: ParallelForFramesPassesCorrectIndices
    {
        const std::size_t task_count = 50;
        std::vector<bool> executed(task_count, false);
        std::mutex mutex;

        tachyon::runtime::TaskflowRuntime runtime(4);
        runtime.parallel_for_frames(task_count, [&](std::size_t index) {
            std::lock_guard<std::mutex> lock(mutex);
            executed[index] = true;
        });

        for (std::size_t i = 0; i < task_count; ++i) {
            if (!executed[i]) {
                g_failures++;
                std::cerr << "FAIL: Task at index " << i << " was not executed\n";
            }
        }
    }

    // Test: ParallelForFramesHandlesSingleThread
    {
        const std::size_t task_count = 10;
        std::atomic<std::size_t> counter{0};

        tachyon::runtime::TaskflowRuntime runtime(1);
        runtime.parallel_for_frames(task_count, [&](std::size_t /*index*/) {
            counter.fetch_add(1);
        });

        check_equals(counter.load(), task_count, "Single thread should execute all tasks");
    }

    return g_failures == 0;
}
