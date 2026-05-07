#include "test_utils.h"
#include <vector>

bool run_math_tests();
bool run_property_tests();
bool run_expression_tests();
bool run_camera_cuts_tests();
bool run_camera_shake_tests();
bool run_bezier_interpolator_tests();
bool run_scene_builder_preset_tests();
bool run_blend_kernel_tests();
bool run_property_sampler_tests();

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"math", run_math_tests},
        {"property", run_property_tests},
        {"expression", run_expression_tests},
        {"camera_cuts", run_camera_cuts_tests},
        {"camera_shake", run_camera_shake_tests},
        {"bezier_interpolator", run_bezier_interpolator_tests},
        {"builder_preset", run_scene_builder_preset_tests},
        {"blend_kernel", run_blend_kernel_tests},
        {"property_sampler", run_property_sampler_tests},
    };

    return run_test_suite(argc, argv, tests);
}
