#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/core/framebuffer.h"
#include "tachyon/renderer2d/color/color_transfer.h"
#include "tachyon/runtime/execution/planning/render_plan.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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

std::filesystem::path write_silence_wav(const std::filesystem::path& path, std::uint32_t sample_rate, std::uint16_t channels, std::uint32_t frames) {
    const std::uint16_t bits_per_sample = 16;
    const std::uint16_t block_align = static_cast<std::uint16_t>(channels * (bits_per_sample / 8U));
    const std::uint32_t byte_rate = sample_rate * block_align;
    const std::uint32_t data_size = frames * block_align;
    const std::uint32_t riff_size = 36U + data_size;

    std::ofstream output(path, std::ios::binary);
    if (!output.is_open()) {
        return {};
    }

    auto write_u16 = [&](std::uint16_t value) {
        output.put(static_cast<char>(value & 0xFFU));
        output.put(static_cast<char>((value >> 8U) & 0xFFU));
    };
    auto write_u32 = [&](std::uint32_t value) {
        output.put(static_cast<char>(value & 0xFFU));
        output.put(static_cast<char>((value >> 8U) & 0xFFU));
        output.put(static_cast<char>((value >> 16U) & 0xFFU));
        output.put(static_cast<char>((value >> 24U) & 0xFFU));
    };

    output.write("RIFF", 4);
    write_u32(riff_size);
    output.write("WAVE", 4);

    output.write("fmt ", 4);
    write_u32(16U);
    write_u16(1U);
    write_u16(channels);
    write_u32(sample_rate);
    write_u32(byte_rate);
    write_u16(block_align);
    write_u16(bits_per_sample);

    output.write("data", 4);
    write_u32(data_size);

    const std::vector<char> silence(data_size, 0);
    output.write(silence.data(), static_cast<std::streamsize>(silence.size()));
    return path;
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

    {
        const auto limited = renderer2d::detail::apply_range_mode(renderer2d::Color{1.0f, 1.0f, 1.0f, 1.0f}, renderer2d::detail::ColorRange::Limited);
        check_true(std::abs(limited.r - 235.0f/255.0f) < 0.001f, "Limited range clamps RGB to legal white");
        const auto matrix = renderer2d::detail::primaries_conversion_matrix(
            renderer2d::detail::ColorPrimaries::Rec709,
            renderer2d::detail::ColorPrimaries::DisplayP3);
        check_true(matrix.m[0] != 0.0f, "Primaries conversion matrix is populated");
    }

    {
        std::filesystem::create_directories("tests/output/frame_sink");
        const std::filesystem::path wav_path = write_silence_wav("tests/output/frame_sink/silence.wav", 44100, 2, 4410);
        check_true(!wav_path.empty(), "synthetic wav fixture was written");
        if (!wav_path.empty()) {
            RenderPlan plan = make_mp4_plan();
            plan.output.destination.path = "tests/output/frame_sink/audio_mux.mp4";
            plan.output.destination.overwrite = true;
            plan.output.profile.audio.mode = "encode";
            plan.output.profile.audio.codec = "aac";
            plan.output.profile.audio.sample_rate = 44100;
            plan.output.profile.audio.channels = 2;
            plan.output.profile.audio.tracks.push_back(tachyon::AudioTrack{
                wav_path.string(),
                1.0,
                0.0
            });

            auto sink = output::create_frame_output_sink(plan);
            check_true(static_cast<bool>(sink), "Audio mux plan resolves to an output sink");
            if (sink) {
                check_true(sink->begin(plan), "Audio mux sink begins successfully");
                renderer2d::Framebuffer frame(8, 8);
                frame.clear(renderer2d::Color::blue());
                check_true(sink->write_frame({0, &frame}), "Audio mux sink writes first frame");
                check_true(sink->finish(), "Audio mux sink finishes cleanly");
                check_true(std::filesystem::exists("tests/output/frame_sink/audio_mux.mp4"), "Audio mux output exists");
            }
        }
    }

    return g_failures == 0;
}
