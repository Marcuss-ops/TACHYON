#include "test_utils.h"
#include <vector>

bool run_scene_evaluator_tests();
bool run_timeline_tests();
bool run_scene_inspector_tests();
bool run_motion_map_tests();
void run_default_camera_tests();
bool run_default_camera_tests_adapter() { run_default_camera_tests(); return true; }

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"scene_evaluator", run_scene_evaluator_tests},
        {"timeline", run_timeline_tests},
        {"scene_inspector", run_scene_inspector_tests},
        {"motion_map", run_motion_map_tests},
        {"default_camera", run_default_camera_tests_adapter},
    };

    return run_test_suite(argc, argv, tests);
}
