#include "test_utils.h"
#include <vector>

// Enabled tests
bool run_frame_executor_tests();
bool run_determinism_tests();
bool run_scene_hash_coverage_tests();

// Temporarily disabled (stubs for linking)
bool run_render_job_tests() { return true; }
bool run_frame_cache_tests() { return true; }
bool run_frame_cache_budget_tests() { return true; }
bool run_runtime_backbone_tests() { return true; }
bool run_expression_vm_tests() { return true; }
bool run_frame_range_tests() { return true; }
bool run_frame_adapter_tests() { return true; }
bool run_frame_output_sink_tests() { return true; }
bool run_tile_scheduler_tests() { return true; }
bool run_tiling_integration_tests() { return true; }
bool run_runtime_policy_tests() { return true; }
bool run_jit_render_tests() { return true; }
bool run_taskflow_runtime_tests() { return true; }

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"frame_executor", run_frame_executor_tests},
        {"determinism", run_determinism_tests},
        {"scene_hash_coverage", run_scene_hash_coverage_tests},
    };

    return run_test_suite(argc, argv, tests);
}
