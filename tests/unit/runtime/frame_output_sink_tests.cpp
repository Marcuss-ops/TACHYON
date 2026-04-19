#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/framebuffer.h"
#include "tachyon/runtime/render_plan.h"

#include <filesystem>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

tachyon::RenderPlan make_png_plan() {
    tachyon::RenderPlan plan;
    plan.job_id = "job_png";
    plan.scene_ref = "scene.json";
    plan.composition_target = "main";
    plan.composition.id = "main";
    plan.composition.name = "Main";
    plan.composition.width = 8;
    plan.composition.height = 8;
    plan.composition.duration = 1.0;
    plan.composition.frame_rate = {30, 1};
    plan.output.destination.path = "tests/output/frame_sink/frame.png";
    plan.output.destination.overwrite = true;
    plan.output.profile.name = "png-seq";
    plan.output.profile.class_name = "png_sequence";
    plan.output.profile.container = "png";
    plan.output.profile.video.codec = "png";
    plan.output.profile.video.pixel_format = "rgba8";
    plan.output.profile.video.rate_control_mode = "fixed";
    plan.output.profile.color.transfer = "srgb";
    plan.output.profile.color.range = "full";
    return plan;
}

tachyon::RenderPlan make_mp4_plan() {
    tachyon::RenderPlan plan = make_png_plan();
    plan.job_id = "job_mp4";
    plan.output.destination.path = "tests/output/frame_sink/out.mp4";
    plan.output.profile.name = "mp4";
    plan.output.profile.class_name = "ffmpeg_pipe";
    plan.output.profile.container = "mp4";
    plan.output.profile.video.codec = "libx264";
    plan.output.profile.video.pixel_format = "yuv420p";
    plan.output.profile.video.rate_control_mode = "crf";
    plan.output.profile.video.crf = 18.0;
    plan.output.profile.audio.mode = "none";
    return plan;
}

} // namespace

bool run_frame_output_sink_tests() {
    using namespace tachyon;

    g_failures = 0;

    {
        const RenderPlan plan = make_png_plan();
        std::filesystem::remove_all("tests/output/frame_sink");

        auto sink = output::create_frame_output_sink(plan);
        check_true(static_cast<bool>(sink), "PNG plan resolves to an output sink");
        if (sink) {
            check_true(sink->begin(plan), "PNG sink begins successfully");
            renderer2d::Framebuffer frame(8, 8);
            frame.clear(renderer2d::Color::green());
            check_true(sink->write_frame({0, &frame}), "PNG sink writes first frame");
            check_true(sink->write_frame({1, &frame}), "PNG sink writes second frame");
            check_true(sink->finish(), "PNG sink finishes cleanly");
            check_true(std::filesystem::exists("tests/output/frame_sink/frame_000001.png"), "First PNG frame exists");
            check_true(std::filesystem::exists("tests/output/frame_sink/frame_000002.png"), "Second PNG frame exists");
        }
    }

    {
        const RenderPlan plan = make_mp4_plan();
        auto sink = output::create_frame_output_sink(plan);
        check_true(static_cast<bool>(sink), "MP4 plan resolves to an output sink");
        check_true(output::output_requests_video_file(plan.output), "MP4 plan selects video file output");
    }

    return g_failures == 0;
}
