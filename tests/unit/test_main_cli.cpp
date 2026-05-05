#include "test_utils.h"
#include <vector>

bool run_cli_tests();
bool run_scene_spec_tests();

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"cli", run_cli_tests},
        {"scene_spec", run_scene_spec_tests},
    };

    return run_test_suite(argc, argv, tests);
}
