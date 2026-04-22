#include "tachyon/runtime/execution/render_job.h"
#include <iostream>

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
        const std::string text = R"({
            "job_id": "job_001",
            "scene_ref": "scene.json",
            "composition_target": "main",
            "frame_range": { "start": 0, "end": 30 }
        })";

        const auto parsed = tachyon::parse_render_job_json(text);
        check_true(parsed.value.has_value(), "canonical render job should parse");
        if (parsed.value.has_value()) {
            const auto validation = tachyon::validate_render_job(*parsed.value);
            check_true(validation.ok(), "canonical render job should validate");
        }
    }
    return g_failures == 0;
}
