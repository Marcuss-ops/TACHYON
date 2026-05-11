#include "tachyon/diagnostics/trace.h"
#include "tachyon/diagnostics/trace_session.h"

#include <iostream>

namespace {

int g_failures = 0;

void test_trace_macros() {
    tachyon::diagnostics::start_json_trace("test_trace.json");

    {
        TACHYON_TRACE_SCOPE("test_scope");
        TACHYON_TRACE_INSTANT("test_instant");
        TACHYON_TRACE_COUNTER("test_counter", 42);
    }

    tachyon::diagnostics::stop_trace();
    // Test that we didn't crash.
    // In a real test, we might want to read "test_trace.json" and verify contents,
    // but for now, just verify that the API doesn't crash when called.
}

} // namespace

bool run_trace_tests() {
    std::cout << "Running Trace tests..." << std::endl;
    g_failures = 0;

    test_trace_macros();

    if (g_failures == 0) {
        std::cout << "All Trace tests passed!" << std::endl;
    } else {
        std::cout << "Trace tests failed with " << g_failures << " errors." << std::endl;
    }

    return g_failures == 0;
}