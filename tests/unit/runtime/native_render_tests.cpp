#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/presets/scene/scene_preset_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/effects/effect_utils.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include <iostream>
#include <array>
#include <string>
#include <filesystem>

namespace tachyon {

namespace {

std::optional<renderer2d::SurfaceRGBA> render_scene_still_surface(
    const SceneSpec& scene,
    const std::string& label,
    double& out_fps) {
    if (scene.compositions.empty()) {
        return std::nullopt;
    }

    const CompositionSpec& composition = scene.compositions.front();
    out_fps = composition.frame_rate.denominator > 0
        ? static_cast<double>(composition.frame_rate.numerator) / static_cast<double>(composition.frame_rate.denominator)
        : 30.0;
    if (out_fps <= 0.0) {
        out_fps = 30.0;
    }

    RenderJob job;
    job.job_id = scene.project.id.empty() ? label + "_still" : scene.project.id + "_still";
    job.scene_ref = label;
    job.composition_target = composition.id;
    job.frame_range = {0, 0};
    job.output.destination.path.clear();
    job.output.destination.overwrite = true;
    job.output.profile.name = "png_sequence";
    job.output.profile.class_name = "image-sequence";
    job.output.profile.container = "png";
    job.output.profile.format = OutputFormat::ImageSequence;
    job.output.profile.alpha_mode = "preserved";
    job.output.profile.video.codec = "png";
    job.output.profile.video.pixel_format = "rgba8";
    job.output.profile.video.rate_control_mode = "fixed";
    job.output.profile.buffering.strategy = "default";
    job.output.profile.color.space = "bt709";
    job.output.profile.color.transfer = "srgb";
    job.output.profile.color.range = "full";

    SceneCompiler compiler;
    const auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok() || !compiled_result.value.has_value()) {
        return std::nullopt;
    }

    const auto plan_result = build_render_plan(scene, job);
    if (!plan_result.value.has_value()) {
        return std::nullopt;
    }
    const auto exec_result = build_render_execution_plan(*plan_result.value, 0);
    if (!exec_result.value.has_value()) {
        return std::nullopt;
    }

    RenderSession session;
    const RenderSessionResult session_result = session.render(scene, *compiled_result.value, *exec_result.value, {});
    if (session_result.frames.empty() || !session_result.frames.front().frame) {
        return std::nullopt;
    }

    return *session_result.frames.front().frame;
}

std::filesystem::path repo_root_path() {
    std::filesystem::path root = std::filesystem::path(TACHYON_TESTS_SOURCE_DIR);
    return root.parent_path();
}

bool render_transition_demo_mp4(
    const std::string& transition_id,
    const renderer2d::SurfaceRGBA& source,
    const renderer2d::SurfaceRGBA& target,
    const std::filesystem::path& output_path) {

    renderer2d::EffectRegistry effect_registry;
    TransitionRegistry transition_registry;
    register_builtin_transitions(transition_registry);
    renderer2d::register_builtin_effects(effect_registry, transition_registry);

    auto host = renderer2d::create_effect_host(effect_registry);

    tachyon::RenderPlan plan;
    plan.job_id = "native-render-light-leak";
    plan.scene_ref = "scene-a-b";
    plan.composition_target = "main";
    plan.composition.id = "main";
    plan.composition.name = "main";
    plan.composition.width = static_cast<std::int64_t>(source.width());
    plan.composition.height = static_cast<std::int64_t>(source.height());
    plan.composition.duration = 3.5;
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

    auto sink = output::create_frame_output_sink(plan);
    if (!sink) {
        return false;
    }
    if (!sink->begin(plan)) {
        return false;
    }

    const double fps = 24.0;
    const std::size_t frame_count = 48;
    const std::size_t lead_in_frames = 4;
    const std::size_t lead_out_frames = 4;
    const std::size_t transition_frames = frame_count - lead_in_frames - lead_out_frames;
    const renderer2d::SurfaceRGBA preview_source = source;
    const renderer2d::SurfaceRGBA preview_target = target;

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

        renderer2d::EffectParams params;
        params.scalars["t"] = t;
        params.scalars["duration_seconds"] = 0.5f;
        params.strings["transition_id"] = transition_id;
        params.aux_surfaces["transition_to"] = &preview_target;

        const auto blended_result = host->apply("tachyon.effect.transition.glsl", preview_source, params);
        if (!blended_result.ok() || !blended_result.value.has_value()) {
            return false;
        }

        output::OutputFramePacket packet;
        packet.frame_number = static_cast<std::int64_t>(index + 1);
        packet.frame = &blended_result.value.value();
        packet.metadata.time_seconds = static_cast<double>(index) / fps;
        packet.metadata.scene_hash = transition_id;
        if (!sink->write_frame(packet)) {
            return false;
        }
    }

    return sink->finish();
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

    std::cout << "[NativeRender] Rendering light leak transition demos...\n";
    struct DemoConfig {
        const char* transition_id;
        const char* file_name;
    };

    const std::array<DemoConfig, 3> light_leak_demos{{
        {"tachyon.transition.light_leak", "light_leak_demo.mp4"},
        {"tachyon.transition.film_burn", "film_burn_demo.mp4"},
        {"tachyon.transition.flash", "flash_demo.mp4"},
    }};

    const std::filesystem::path demo_dir = repo_root_path() / "output" / "light_leaks";
    std::filesystem::create_directories(demo_dir);

    double source_fps = 30.0;
    double target_fps = 30.0;
    auto source_scene = presets::ScenePresetRegistry::instance().create("tachyon.scene.a", {});
    auto target_scene = presets::ScenePresetRegistry::instance().create("tachyon.scene.b", {});
    if (!source_scene.has_value() || !target_scene.has_value()) {
        std::cerr << "[NativeRender] FAIL: scene preset lookup failed\n";
        return false;
    }

    auto source_still = render_scene_still_surface(*source_scene, "tachyon.scene.a", source_fps);
    auto target_still = render_scene_still_surface(*target_scene, "tachyon.scene.b", target_fps);
    if (!source_still.has_value() || !target_still.has_value()) {
        std::cerr << "[NativeRender] FAIL: still render failed\n";
        return false;
    }

    for (const auto& demo : light_leak_demos) {
        const std::filesystem::path mp4_path = demo_dir / demo.file_name;

        std::filesystem::remove(mp4_path);
        if (!render_transition_demo_mp4(demo.transition_id, *source_still, *target_still, mp4_path)) {
            std::cerr << "[NativeRender] FAIL: transition render failed for " << demo.transition_id << "\n";
            return false;
        }

        if (!std::filesystem::exists(mp4_path)) {
            std::cerr << "[NativeRender] FAIL: rendered MP4 missing for " << demo.transition_id << "\n";
            return false;
        }

        std::cout << "[OK] Rendered " << demo.transition_id << " to " << mp4_path << "\n";
    }
    return true;
}

} // namespace tachyon
