#include "test_utils.h"
#include <vector>

namespace tachyon::output::test {
bool run_ffmpeg_caps_tests();
bool run_hardware_encoder_detector_tests();
}

bool run_shared_memory_sink_tests();

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"ffmpeg_caps", tachyon::output::test::run_ffmpeg_caps_tests},
        {"hardware_encoder_detector", tachyon::output::test::run_hardware_encoder_detector_tests},
        {"shared_memory_sink", run_shared_memory_sink_tests},
    };

    return run_test_suite(argc, argv, tests);
}
