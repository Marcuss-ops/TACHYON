#include "test_utils.h"
#include <vector>

bool run_render_job_tests();
bool run_frame_cache_tests();
bool run_frame_cache_budget_tests();
bool run_runtime_backbone_tests();
bool run_expression_vm_tests();
bool run_frame_executor_tests();
bool run_frame_range_tests();
bool run_frame_adapter_tests();
bool run_frame_output_sink_tests();
bool run_tile_scheduler_tests();
bool run_tiling_integration_tests();

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"render_job", run_render_job_tests},
        {"frame_cache", run_frame_cache_tests},
        {"frame_cache_budget", run_frame_cache_budget_tests},
        {"runtime_backbone", run_runtime_backbone_tests},
        {"expression_vm", run_expression_vm_tests},
        {"frame_executor", run_frame_executor_tests},
        {"frame_range", run_frame_range_tests},
        {"frame_adapter", run_frame_adapter_tests},
        {"frame_output_sink", run_frame_output_sink_tests},
        {"tile_scheduler", run_tile_scheduler_tests},
        {"tiling_integration", run_tiling_integration_tests},
    };

    return run_test_suite(argc, argv, tests);
}
