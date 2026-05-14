#include "test_utils.h"
#include <vector>

// Enabled tests
bool run_frame_executor_tests();
bool run_determinism_tests();
bool run_scene_hash_coverage_tests();

// Temporarily disabled (stubs for linking)
bool run_render_job_tests();
bool run_frame_cache_tests();
bool run_frame_cache_budget_tests();
bool run_runtime_backbone_tests();
bool run_expression_vm_tests();
bool run_frame_range_tests();
bool run_frame_adapter_tests();
bool run_frame_output_sink_tests();
bool run_tile_scheduler_tests();
bool run_tiling_integration_tests();
bool run_runtime_policy_tests();
bool run_jit_render_tests();
bool run_taskflow_runtime_tests();

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"frame_executor", run_frame_executor_tests},
        {"determinism", run_determinism_tests},
        {"scene_hash_coverage", run_scene_hash_coverage_tests},
    };

    return run_test_suite(argc, argv, tests);
}
