#include "test_utils.h"
#include <vector>

namespace tachyon::core::assets {
bool run_asset_resolution_tests();
}

namespace tachyon::runtime::simd {
bool run_simd_blend_tests();
}

namespace tachyon::test {
bool run_c_api_smoke_test();
}

namespace tachyon::diagnostics {
bool run_trace_scope_test();
bool run_trace_counter_test();
bool run_trace_session_test();
bool run_logging_test();
}

int main(int argc, char** argv) {
    using namespace tachyon::test;
    std::vector<TestCase> tests = {
        {"asset_resolution", tachyon::core::assets::run_asset_resolution_tests},
        {"c_api_smoke", tachyon::test::run_c_api_smoke_test},
        {"simd_blend", tachyon::runtime::simd::run_simd_blend_tests},
        {"trace_session", tachyon::diagnostics::run_trace_session_test},
        {"logging_json", tachyon::diagnostics::run_logging_test},
#if defined(TACHYON_ENABLE_TRACE)
        {"trace_scope", tachyon::diagnostics::run_trace_scope_test},
        {"trace_counter", tachyon::diagnostics::run_trace_counter_test},
#endif
    };

    return run_test_suite(argc, argv, tests);
}
