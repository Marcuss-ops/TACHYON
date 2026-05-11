#include "tachyon/diagnostics/trace.h"
#include "tachyon/diagnostics/trace_session.h"

#include <iostream>

namespace {

int g_failures = 0;

void test_trace_macros_json() {
    tachyon::diagnostics::start_json_trace("test_trace.json");

    {
        TACHYON_TRACE_SCOPE("test_scope_json");
        TACHYON_TRACE_INSTANT("test_instant_json");
        TACHYON_TRACE_COUNTER("test_counter_json", 42);
    }

    tachyon::diagnostics::stop_trace();
}

void test_trace_macros_perfetto() {
    tachyon::diagnostics::start_perfetto_trace("test_trace.perfetto-trace");

    {
        TACHYON_TRACE_SCOPE("test_scope_perfetto");
        TACHYON_TRACE_INSTANT("test_instant_perfetto");
        TACHYON_TRACE_COUNTER("test_counter_perfetto", 43);
    }

    tachyon::diagnostics::stop_trace();
}

} // namespace

bool run_trace_tests() {
    std::cout << "Running Trace tests..." << std::endl;
    g_failures = 0;

    test_trace_macros_json();
    test_trace_macros_perfetto();

    if (g_failures == 0) {
        std::cout << "All Trace tests passed!" << std::endl;
    } else {
        std::cout << "Trace tests failed with " << g_failures << " errors." << std::endl;
    }

    return g_failures == 0;
}