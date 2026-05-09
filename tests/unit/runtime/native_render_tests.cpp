#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/core/transition/transition_effect_resolver.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/renderer2d/surface/surface_sampling.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include <iostream>
#include <array>
#include <algorithm>
#include <chrono>
#include <string>
#include <filesystem>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace tachyon {

namespace {

class NullFrameOutputSink final : public output::FrameOutputSink {
public:
    bool begin(const RenderPlan&) override { return true; }
    bool write_frame(const output::OutputFramePacket&) override { return true; }
    bool finish() override { return true; }
    const std::string& last_error() const override {
        static const std::string kEmpty;
        return kEmpty;
    }
};

std::optional<renderer2d::SurfaceRGBA> load_video_frame_surface(
    media::MediaManager& media_manager,
    const std::filesystem::path& video_path,
    double time,
    std::ostream& err) {
    DiagnosticBag diagnostics;
    const auto* frame = media_manager.get_video_frame(video_path, time, &diagnostics);
    if (!frame) {
        err << "[NativeRender] FAIL: could not load video frame from " << video_path.string() << '\n';
        return std::nullopt;
    }
    return *frame;
}

std::filesystem::path repo_root_path() {
    std::filesystem::path root = std::filesystem::path(TACHYON_TESTS_SOURCE_DIR);
    return root.parent_path();
}

renderer2d::SurfaceRGBA resize_surface(const renderer2d::SurfaceRGBA& src, std::uint32_t width, std::uint32_t height) {
    renderer2d::SurfaceRGBA out(width, height);
    out.set_profile(src.profile());

    if (width == 0U || height == 0U) {
        return out;
    }

    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(width);
            const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(height);
            out.set_pixel(x, y, renderer2d::sample_texture_bilinear(src, u, v, renderer2d::Color::white()));
        }
    }

    return out;
}

std::array<double, 3> sample_average_rgb(const renderer2d::SurfaceRGBA& surface) {
    double sum_r = 0.0;
    double sum_g = 0.0;
    double sum_b = 0.0;
    std::size_t samples = 0;

    const std::uint32_t step_x = std::max<std::uint32_t>(1U, surface.width() / 16U);
    const std::uint32_t step_y = std::max<std::uint32_t>(1U, surface.height() / 16U);
    for (std::uint32_t y = 0; y < surface.height(); y += step_y) {
        for (std::uint32_t x = 0; x < surface.width(); x += step_x) {
            const auto c = surface.get_pixel(x, y);
            sum_r += c.r;
            sum_g += c.g;
            sum_b += c.b;
            ++samples;
        }
    }

    if (samples == 0) {
        return {0.0, 0.0, 0.0};
    }

    return {
        sum_r / static_cast<double>(samples),
        sum_g / static_cast<double>(samples),
        sum_b / static_cast<double>(samples)
    };
}

RenderPlan make_transition_render_plan(
    const renderer2d::SurfaceRGBA& source,
    const std::filesystem::path& output_path) {
    RenderPlan plan;
    plan.job_id = "native-render-transition-benchmark";
    plan.scene_ref = "scene-a-b";
    plan.composition_target = "main";
    plan.composition.id = "main";
    plan.composition.name = "main";
    plan.composition.width = static_cast<std::int64_t>(source.width());
    plan.composition.height = static_cast<std::int64_t>(source.height());
    plan.composition.duration = 4.0;
    plan.composition.frame_rate = {24, 1};
    plan.output.destination.path = output_path.string();
    plan.output.destination.overwrite = true;
    plan.output.profile.name = "h264-mp4";
    plan.output.profile.class_name = "video";
    plan.output.profile.container = "mp4";
    plan.output.profile.alpha_mode = "opaque";
    plan.output.profile.video.codec = "libx264";
    plan.output.profile.video.pixel_format = "yuv420p";
    plan.output.profile.video.rate_control_mode = "fixed";
    plan.output.profile.buffering.strategy = "default";
    plan.output.profile.color.space = "bt709";
    plan.output.profile.color.transfer = "srgb";
    plan.output.profile.color.range = "full";
    plan.output.profile.audio.mode = "none";
    return plan;
}

bool render_transition_demo_with_sink(
    const std::string& transition_id,
    const TransitionKernel& kernel,
    const renderer2d::SurfaceRGBA& source,
    const renderer2d::SurfaceRGBA& target,
    const RenderPlan& plan,
    output::FrameOutputSink& sink,
    const char* mode_label) {
    const auto render_started = std::chrono::steady_clock::now();
    if (!sink.begin(plan)) {
        return false;
    }

    const double fps = 24.0;
    const std::size_t frame_count = 96;
    const std::size_t lead_in_frames = 24;
    const std::size_t lead_out_frames = 24;
    const std::size_t transition_frames = frame_count - lead_in_frames - lead_out_frames;
    const renderer2d::SurfaceRGBA& preview_source = source;
    const renderer2d::SurfaceRGBA& preview_target = target;

    for (std::size_t index = 0; index < frame_count; ++index) {
        float t = 0.0f;
        if (index < lead_in_frames) {
            t = 0.0f;
        } else if (index < lead_in_frames + transition_frames) {
            const std::size_t local_index = index - lead_in_frames;
            const double normalized = transition_frames <= 1
                ? 1.0
                : static_cast<double>(local_index) / static_cast<double>(transition_frames - 1);
            t = static_cast<float>(normalized);
        } else {
            t = 1.0f;
        }

        const renderer2d::SurfaceRGBA blended_result = kernel.apply(preview_source, &preview_target, t);
        if (index == 0 || index == lead_in_frames || index + 1 == frame_count) {
            const auto avg = sample_average_rgb(blended_result);
            std::cout << "[NativeRender] " << mode_label << " " << transition_id
                      << " frame=" << (index + 1)
                      << " t=" << t
                      << " avg_rgb=(" << avg[0] << "," << avg[1] << "," << avg[2] << ")\n";
        }

        output::OutputFramePacket packet;
        packet.frame_number = static_cast<std::int64_t>(index + 1);
        packet.frame = &blended_result;
        packet.metadata.time_seconds = static_cast<double>(index) / fps;
        packet.metadata.scene_hash = transition_id;
        if (!sink.write_frame(packet)) {
            return false;
        }
    }

    if (!sink.finish()) {
        return false;
    }

    const auto render_finished = std::chrono::steady_clock::now();
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(render_finished - render_started).count();
    std::cout << "[NativeRender] " << mode_label << " " << transition_id
              << " total_ms=" << elapsed_ms
              << " frames=" << frame_count
              << " resolution=" << preview_source.width() << "x" << preview_source.height()
              << " output=" << plan.output.destination.path
              << '\n';

    return true;
}

} // namespace

bool run_native_render_tests() {

    {
        SceneSpec scene;
        scene.project.id = "preflight_scene";
        scene.project.name = "Preflight Scene";

        CompositionSpec comp;
        comp.id = "main";
        comp.width = 1280;
        comp.height = 720;
        comp.duration = 1.0;
        comp.frame_rate = {30, 1};
        scene.compositions.push_back(comp);

        RenderJob job;
        job.job_id = "preflight_invalid";
        job.scene_ref = "preflight_scene";
        job.composition_target = "main";
        job.frame_range = {0, 0};

        const auto result = NativeRenderer::render(scene, job);
        if (result.diagnostics.ok()) {
            std::cerr << "FAIL: preflight should reject an invalid render job\n";
            return false;
        }
        if (result.scene_compile_ms != 0.0 || result.plan_build_ms != 0.0 || result.execution_plan_build_ms != 0.0) {
            std::cerr << "FAIL: preflight should short-circuit before compilation\n";
            return false;
        }
    }

    std::cout << "[NativeRender] Benchmarking the heaviest transition from clip_a/clip_b...\n";
#ifdef _OPENMP
    std::cout << "[NativeRender] OpenMP max_threads=" << omp_get_max_threads()
              << " num_procs=" << omp_get_num_procs() << '\n';
#endif
    const std::string transition_id = "tachyon.transition.soft_zoom_blur";

    const std::filesystem::path demo_dir = repo_root_path() / "output" / "transitions" / "clip_pair_demos";
    std::filesystem::create_directories(demo_dir);

    media::MediaManager media_manager;
    const std::filesystem::path source_video = repo_root_path() / "output" / "transitions" / "clip_a.mp4";
    const std::filesystem::path target_video = repo_root_path() / "output" / "transitions" / "clip_b.mp4";
    auto source_still = load_video_frame_surface(media_manager, source_video, 0.0, std::cerr);
    auto target_still = load_video_frame_surface(media_manager, target_video, 0.0, std::cerr);
    if (!source_still.has_value() || !target_still.has_value()) {
        std::cerr << "[NativeRender] FAIL: video still load failed\n";
        return false;
    }

    TransitionRegistry transition_registry;
    register_builtin_transitions(transition_registry);
    for (auto* desc : transition_registry.list_all()) {
        if (!desc) continue;
        renderer2d::resolve_basic_transition_implementations(const_cast<TransitionDescriptor&>(*desc));
        renderer2d::resolve_artistic_transition_implementations(const_cast<TransitionDescriptor&>(*desc));
        renderer2d::resolve_light_leak_implementations(const_cast<TransitionDescriptor&>(*desc));
    }
    TransitionEffectResolver resolver{transition_registry};
    TransitionEffectRequest request;
    request.transition_id = transition_id;
    request.preferred_backend = TransitionRuntimeKind::CpuPixel;
    request.fallback_policy = TransitionFallbackPolicy::Strict;
    const auto resolved = resolver.resolve(request);
    if (!resolved.valid || !resolved.kernel.valid || !resolved.kernel.apply) {
        std::cerr << "[NativeRender] FAIL: transition kernel resolve failed for " << transition_id << '\n';
        return false;
    }

    const std::filesystem::path mp4_path = demo_dir / "soft_zoom_blur_benchmark.mp4";
    const auto plan = make_transition_render_plan(*source_still, mp4_path);

    NullFrameOutputSink null_sink;
    if (!render_transition_demo_with_sink(transition_id, resolved.kernel, *source_still, *target_still, plan, null_sink, "compute-only")) {
        std::cerr << "[NativeRender] FAIL: compute-only benchmark failed for " << transition_id << "\n";
        return false;
    }

    std::filesystem::remove(mp4_path);
    auto sink = output::create_frame_output_sink(plan);
    if (!sink) {
        std::cerr << "[NativeRender] FAIL: could not create MP4 sink for " << transition_id << "\n";
        return false;
    }

    if (!render_transition_demo_with_sink(transition_id, resolved.kernel, *source_still, *target_still, plan, *sink, "mp4")) {
        std::cerr << "[NativeRender] FAIL: transition render failed for " << transition_id << "\n";
        return false;
    }

    if (!std::filesystem::exists(mp4_path)) {
        std::cerr << "[NativeRender] FAIL: rendered MP4 missing for " << transition_id << "\n";
        return false;
    }
    std::cout << "[OK] Rendered " << transition_id << " to " << mp4_path << "\n";
    return true;
}

} // namespace tachyon
