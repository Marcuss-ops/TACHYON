#include "test_utils.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include <iostream>

using namespace tachyon;

bool run_render_job_tests() {
    std::cout << "[RenderJobTests] Starting RenderJob unit tests...\n";

    // Test 1: Still image job builder
    {
        auto job = RenderJobBuilder::still_image("comp_1", 10, "/out/still.png");
        if (job.frame_range.start != 10 || job.frame_range.end != 10) {
            std::cerr << "[RenderJobTests] FAIL: Still image frame range incorrect\n";
            return false;
        }
        if (job.output.profile.name != "png_sequence") {
            std::cerr << "[RenderJobTests] FAIL: Still image profile name incorrect\n";
            return false;
        }
        if (job.output.profile.format != OutputFormat::ImageSequence) {
            std::cerr << "[RenderJobTests] FAIL: Still image profile format incorrect\n";
            return false;
        }
        std::cout << "[RenderJobTests] Still image job verified!\n";
    }

    // Test 2: Video export job builder
    {
        FrameRange range{0, 60};
        auto job = RenderJobBuilder::video_export("comp_2", range, "/out/video.mp4");
        if (job.frame_range.start != 0 || job.frame_range.end != 60) {
            std::cerr << "[RenderJobTests] FAIL: Video export frame range incorrect\n";
            return false;
        }
        if (job.output.profile.video.codec != "libx264") {
            std::cerr << "[RenderJobTests] FAIL: Video export codec incorrect\n";
            return false;
        }
        std::cout << "[RenderJobTests] Video export job verified!\n";
    }

    // Test 3: Resolve output profile
    {
        auto png_profile = resolve_output_profile("png");
        if (png_profile.name != "png_sequence" || png_profile.format != OutputFormat::ImageSequence) {
            std::cerr << "[RenderJobTests] FAIL: Profile resolve 'png' failed\n";
            return false;
        }

        auto mp4_profile = resolve_output_profile("mp4");
        if (mp4_profile.name != "h264_high" || mp4_profile.format != OutputFormat::Video) {
            std::cerr << "[RenderJobTests] FAIL: Profile resolve 'mp4' failed\n";
            return false;
        }

        std::cout << "[RenderJobTests] Output profile resolution verified!\n";
    }

    std::cout << "[RenderJobTests] ALL TESTS PASSED!\n";
    return true;
}
