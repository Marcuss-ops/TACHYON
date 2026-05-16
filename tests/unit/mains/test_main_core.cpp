#include "test_utils.h"
#include <vector>

namespace tachyon::core::assets {
bool run_asset_resolution_tests();
}

namespace tachyon::diagnostics {
bool run_trace_scope_test();
bool run_trace_counter_test();
bool run_trace_session_test();
}

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"asset_resolution", tachyon::core::assets::run_asset_resolution_tests},
        {"trace_session", tachyon::diagnostics::run_trace_session_test},
#if defined(TACHYON_ENABLE_TRACE)
        {"trace_scope", tachyon::diagnostics::run_trace_scope_test},
        {"trace_counter", tachyon::diagnostics::run_trace_counter_test},
#endif
    };

    return run_test_suite(argc, argv, tests);
}
