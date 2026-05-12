#include "test_utils.h"
#include <vector>

bool run_render_session_tests();
bool run_parallax_cards_tests();
bool run_motion_blur_tests();
bool run_time_remap_tests();
bool run_frame_blend_tests();
bool run_rolling_shutter_tests();

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"render_session", run_render_session_tests},
        {"parallax_cards", run_parallax_cards_tests},
        {"motion_blur", run_motion_blur_tests},
        {"time_remap", run_time_remap_tests},
        {"frame_blend", run_frame_blend_tests},
        {"rolling_shutter", run_rolling_shutter_tests},
    };

    return run_test_suite(argc, argv, tests);
}
