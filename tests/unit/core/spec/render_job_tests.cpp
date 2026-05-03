#include "tachyon/runtime/execution/jobs/render_job.h"
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
    
    // Test manual construction (starting from builder to get defaults)
    {
        auto job = tachyon::RenderJobBuilder::still_image("comp1", 0, "output.png");
        job.job_id = "manual_test_job";
        
        const auto validation = tachyon::validate_render_job(job);
        check_true(validation.ok(), "fully populated job should validate");
    }

    // Test RenderJobBuilder
    {
        auto job = tachyon::RenderJobBuilder::video_export("comp1", {0, 100}, "output.mp4");
        check_true(job.composition_target == "comp1", "builder set correct composition");
        check_true(job.frame_range.start == 0 && job.frame_range.end == 100, "builder set correct frame range");
        
        const auto validation = tachyon::validate_render_job(job);
        check_true(validation.ok(), "builder constructed job should validate");
    }

    return g_failures == 0;
}
