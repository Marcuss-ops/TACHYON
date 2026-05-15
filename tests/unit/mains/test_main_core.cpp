#include "test_utils.h"
#include <vector>

namespace tachyon::core::assets {
bool run_asset_resolution_tests();
}

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"asset_resolution", tachyon::core::assets::run_asset_resolution_tests},
    };

    return run_test_suite(argc, argv, tests);
}
