#include "test_utils.h"
#include <vector>

namespace tachyon::backends {
bool run_backend_registry_tests();
}

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"backend_registry", tachyon::backends::run_backend_registry_tests},
    };

    return run_test_suite(argc, argv, tests);
}
