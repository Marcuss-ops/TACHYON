#include "test_utils.h"
#include <vector>

namespace tachyon::test {
bool run_deterministic_single_frame_test();
bool run_deterministic_sequence_test();
bool run_cache_determinism_test();
bool run_parallel_determinism_test();
}

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"single_frame", run_deterministic_single_frame_test},
        {"sequence", run_deterministic_sequence_test},
        {"cache", run_cache_determinism_test},
        {"parallel", run_parallel_determinism_test},
    };

    return run_test_suite(argc, argv, tests);
}
