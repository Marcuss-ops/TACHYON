#include "test_utils.h"
#include <vector>

bool run_scene_evaluator_tests();
bool run_ae_builder_tests();
bool run_precomp_mask_tests();
bool run_timeline_tests();
bool run_scene_inspector_tests();
bool run_motion_map_tests();

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"scene_evaluator", run_scene_evaluator_tests},
        {"ae_builder", run_ae_builder_tests},
        {"precomp_mask", run_precomp_mask_tests},
        {"timeline", run_timeline_tests},
        {"scene_inspector", run_scene_inspector_tests},
        {"motion_map", run_motion_map_tests},
    };

    return run_test_suite(argc, argv, tests);
}
