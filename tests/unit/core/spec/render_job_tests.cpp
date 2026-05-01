#include "tachyon/runtime/execution/jobs/render_job.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
int g_failures = 0;
void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}
}

bool run_render_job_tests() {
    g_failures = 0;
    {
        std::ifstream input("tests/fixtures/jobs/canonical_render_job.json", std::ios::in | std::ios::binary);
        check_true(input.is_open(), "canonical render job fixture opens");
        std::stringstream buffer;
        buffer << input.rdbuf();
        const std::string text = buffer.str();

        const auto parsed = tachyon::parse_render_job_json(text);
        check_true(parsed.value.has_value(), "canonical render job should parse");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_render_job(*parsed.value);
            check_true(validation.ok(), "canonical render job should validate");
        }
    }
    return g_failures == 0;
}
