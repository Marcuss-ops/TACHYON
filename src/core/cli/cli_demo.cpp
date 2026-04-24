#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/effects/effect_utils.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/studio/studio_library.h"
#include "cli_internal.h"

#include <cmath>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <sstream>
#include <nlohmann/json.hpp>

namespace tachyon {
namespace {

struct TransitionDemoConfig {
    std::string transition_id;
    std::string name;
    std::filesystem::path source_scene;
    std::filesystem::path target_scene;
    std::filesystem::path output_dir;
    std::string file_prefix;
    std::string output_format{"mp4"};
    double duration_seconds{2.0};
    double progress_start{0.0};
    double progress_end{1.0};
    std::size_t preview_frame_count{12};
    float preview_resolution_scale{1.0f};
};

nlohmann::json read_json_file(const std::filesystem::path& path, DiagnosticBag& diagnostics) {
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        diagnostics.add_error("studio.demo.open_failed", "failed to open demo file", path.string());
        return {};
    }

    try {
        return nlohmann::json::parse(input);
    } catch (const nlohmann::json::parse_error& error) {
        diagnostics.add_error("studio.demo.parse_failed", error.what(), path.string());
        return {};
    }
}

std::filesystem::path resolve_library_root(const std::filesystem::path& requested_root) {
    if (!requested_root.empty()) {
        return std::filesystem::absolute(requested_root);
    }

    const std::filesystem::path current = std::filesystem::current_path();
    const std::vector<std::filesystem::path> candidates = {
        current / "studio" / "library",
        current.parent_path() / "studio" / "library",
        current.parent_path().parent_path() / "studio" / "library"
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate / "system" / "manifest.json")) {
            return std::filesystem::absolute(candidate);
        }
    }

    return std::filesystem::absolute(current / "studio" / "library");
}

std::optional<TransitionDemoConfig> load_transition_demo(
    const StudioTransitionEntry& entry,
    DiagnosticBag& diagnostics) {
    const nlohmann::json demo = read_json_file(entry.demo_path, diagnostics);
    if (!demo.is_object()) {
        return std::nullopt;
    }

    TransitionDemoConfig config;
    config.transition_id = entry.id;
    config.name = demo.value("name", entry.name);

    const std::filesystem::path demo_dir = entry.demo_path.parent_path();
    config.source_scene = demo_dir / demo.value("source_scene", std::string{});
    config.target_scene = demo_dir / demo.value("target_scene", std::string{});
    config.output_dir = entry.output_dir;
    config.file_prefix = demo.value("output", nlohmann::json{}).value("file_prefix", entry.id);
    config.output_format = demo.value("output", nlohmann::json{}).value("format", std::string{"mp4"});
    config.duration_seconds = demo.value("transition", nlohmann::json{}).value("duration_seconds", entry.duration_seconds);
    const auto& progress = demo.value("transition", nlohmann::json{}).value("progress", nlohmann::json::object());
    config.progress_start = progress.value("start", 0.0);
    config.progress_end = progress.value("end", 1.0);
    const auto preview = demo.value("preview", nlohmann::json::object());
    config.preview_frame_count = std::max<std::size_t>(2, preview.value("frame_count", static_cast<std::size_t>(12)));
    config.preview_resolution_scale = static_cast<float>(std::clamp(preview.value("resolution_scale", 1.0), 0.1, 1.0));
    return config;
}

std::optional<renderer2d::SurfaceRGBA> render_scene_still(
    const std::filesystem::path& scene_path,
    std::ostream& err,
    double& out_fps) {
    SceneSpec scene;
    AssetResolutionTable assets;
    if (!load_scene_context(scene_path, scene, assets, err)) {
        return std::nullopt;
    }

    if (scene.compositions.empty()) {
        err << "scene has no compositions: " << scene_path.string() << '\n';
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
    job.job_id = scene.project.id.empty() ? scene_path.stem().string() + "_demo" : scene.project.id + "_demo";
    job.scene_ref = scene_path.generic_string();
    job.composition_target = composition.id;
    job.frame_range = {0, 0};
    job.output.destination.path.clear();
    job.output.destination.overwrite = true;
    job.output.profile.name = "png-sequence";
    job.output.profile.class_name = "png-sequence";
    job.output.profile.container = "png";
    job.output.profile.alpha_mode = "preserved";
    job.output.profile.video.codec = "png";
    job.output.profile.video.pixel_format = "rgba8";
    job.output.profile.video.rate_control_mode = "fixed";
    job.output.profile.color.space = "bt709";
    job.output.profile.color.transfer = "srgb";
    job.output.profile.color.range = "full";

    SceneCompiler compiler;
    const auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok() || !compiled_result.value.has_value()) {
        print_diagnostics(compiled_result.diagnostics, err);
        return std::nullopt;
    }

    const auto plan_result = build_render_plan(scene, job);
    if (!plan_result.value.has_value()) {
        print_diagnostics(plan_result.diagnostics, err);
        return std::nullopt;
    }
    const auto exec_result = build_render_execution_plan(*plan_result.value, 0);
    if (!exec_result.value.has_value()) {
        print_diagnostics(exec_result.diagnostics, err);
        return std::nullopt;
    }

    RenderSession session;
    const RenderSessionResult session_result = session.render(scene, *compiled_result.value, *exec_result.value, {});
    if (session_result.frames.empty() || !session_result.frames.front().frame) {
        err << "scene rendered no frames: " << scene_path.string() << '\n';
        return std::nullopt;
    }

    return *session_result.frames.front().frame;
}

renderer2d::SurfaceRGBA resize_surface(const renderer2d::SurfaceRGBA& src, std::uint32_t width, std::uint32_t height) {
    if (src.width() == width && src.height() == height) {
        return src;
    }

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

renderer2d::SurfaceRGBA preview_surface(const renderer2d::SurfaceRGBA& src, float scale) {
    const float clamped_scale = std::clamp(scale, 0.1f, 1.0f);
    const std::uint32_t width = std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(std::lround(static_cast<float>(src.width()) * clamped_scale)));
    const std::uint32_t height = std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(std::lround(static_cast<float>(src.height()) * clamped_scale)));
    return resize_surface(src, width, height);
}

RenderPlan make_output_plan(const std::filesystem::path& output_dir, const std::string& prefix) {
    RenderPlan plan;
    plan.job_id = "studio-demo";
    plan.scene_ref = "studio/library";
    plan.composition_target = prefix;
    plan.composition.id = prefix;
    plan.composition.name = prefix;
    plan.composition.width = 1;
    plan.composition.height = 1;
    plan.composition.duration = 1.0;
    plan.composition.frame_rate = {30, 1};
    plan.output.destination.path = (output_dir / (prefix + ".png")).generic_string();
    plan.output.destination.overwrite = true;
    plan.output.profile.name = "png-sequence";
    plan.output.profile.class_name = "png-sequence";
    plan.output.profile.container = "png";
    plan.output.profile.alpha_mode = "preserved";
    plan.output.profile.video.codec = "png";
    plan.output.profile.video.pixel_format = "rgba8";
    plan.output.profile.video.rate_control_mode = "fixed";
    plan.output.profile.color.space = "bt709";
    plan.output.profile.color.transfer = "srgb";
    plan.output.profile.color.range = "full";
    return plan;
}

bool render_transition_demo(
    const TransitionDemoConfig& demo,
    const std::filesystem::path& transition_shader_path,
    const std::filesystem::path& output_dir_override,
    std::ostream& out,
    std::ostream& err) {
    double source_fps = 30.0;
    const auto source_frame = render_scene_still(demo.source_scene, err, source_fps);
    if (!source_frame.has_value()) {
        return false;
    }
    double target_fps = 30.0;
    const auto target_frame = render_scene_still(demo.target_scene, err, target_fps);
    if (!target_frame.has_value()) {
        return false;
    }

    const renderer2d::SurfaceRGBA source = *source_frame;
    const renderer2d::SurfaceRGBA target = resize_surface(*target_frame, source.width(), source.height());
    const bool final_video = demo.output_format == "mp4" || demo.output_format == "mov" || demo.output_format == "mkv" || demo.output_format == "webm";
    const renderer2d::SurfaceRGBA preview_source = preview_surface(source, demo.preview_resolution_scale);
    const renderer2d::SurfaceRGBA preview_target = preview_surface(target, demo.preview_resolution_scale);

    auto host = renderer2d::create_effect_host();
    renderer2d::EffectHost::register_builtins(*host);

    const std::filesystem::path final_output_dir = output_dir_override.empty() ? demo.output_dir : output_dir_override;
    RenderPlan output_plan = make_output_plan(final_output_dir, demo.file_prefix);
    output_plan.composition.width = static_cast<std::int64_t>(preview_source.width());
    output_plan.composition.height = static_cast<std::int64_t>(preview_source.height());
    if (final_video) {
        output_plan.output.destination.path = (final_output_dir / (demo.file_prefix + ".mp4")).generic_string();
        output_plan.output.profile.class_name = "video";
        output_plan.output.profile.container = "mp4";
        output_plan.output.profile.video.codec = "libx264";
        output_plan.output.profile.video.pixel_format = "yuv420p";
        output_plan.output.profile.video.rate_control_mode = "fixed";
        output_plan.output.profile.alpha_mode = "opaque";
        output_plan.composition.frame_rate = {30, 1};
    }
    auto sink = output::create_frame_output_sink(output_plan);
    if (!sink) {
        err << "failed to create png sequence sink for " << demo.output_dir.string() << '\n';
        return false;
    }

    if (!sink->begin(output_plan)) {
        err << sink->last_error() << '\n';
        return false;
    }

    const std::size_t frame_count = final_video
        ? std::max<std::size_t>(2, static_cast<std::size_t>(demo.duration_seconds * 30.0))
        : std::max<std::size_t>(2, demo.preview_frame_count);
    const double progress_span = demo.progress_end - demo.progress_start;
    std::size_t written = 0;

    for (std::size_t index = 0; index < frame_count; ++index) {
        const double normalized = frame_count == 1 ? 1.0 : static_cast<double>(index) / static_cast<double>(frame_count - 1);
        const float t = static_cast<float>(demo.progress_start + progress_span * normalized);

        renderer2d::EffectParams params;
        params.scalars["t"] = t;
        params.scalars["duration_seconds"] = static_cast<float>(demo.duration_seconds);
        params.strings["transition_id"] = demo.transition_id;
        params.strings["shader_path"] = transition_shader_path.generic_string();
        params.aux_surfaces["transition_to"] = &preview_target;

        const renderer2d::SurfaceRGBA blended = host->apply("glsl_transition", preview_source, params);

        output::OutputFramePacket packet;
        packet.frame_number = static_cast<std::int64_t>(index + 1);
        packet.frame = &blended;
        packet.metadata.time_seconds = demo.duration_seconds * normalized;
        packet.metadata.timecode = std::to_string(index + 1);
        packet.metadata.scene_hash = demo.transition_id;

        if (!sink->write_frame(packet)) {
            err << sink->last_error() << '\n';
            return false;
        }
        ++written;
    }

    if (!sink->finish()) {
        err << sink->last_error() << '\n';
        return false;
    }

    out << demo.transition_id << ": wrote " << written << " frame(s) to " << final_output_dir.generic_string() << '\n';
    return true;
}

}  // namespace

bool run_studio_demo_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    const std::filesystem::path library_root = resolve_library_root(options.library_path);
    StudioLibrary library(library_root);
    if (!library.ok()) {
        print_diagnostics(library.diagnostics(), err);
        return false;
    }

    std::vector<StudioTransitionEntry> transitions;
    if (options.transition_id.has_value()) {
        const auto transition = library.find_transition(*options.transition_id);
        if (!transition.has_value()) {
            err << "transition not found: " << *options.transition_id << '\n';
            return false;
        }
        transitions.push_back(*transition);
    } else {
        transitions = library.transitions();
    }

    if (transitions.empty()) {
        err << "no transitions available in " << library_root.generic_string() << '\n';
        return false;
    }

    std::ostringstream silent_out;
    std::ostream& demo_out = options.json_output ? static_cast<std::ostream&>(silent_out) : out;

    bool all_ok = true;
    for (const auto& transition : transitions) {
        DiagnosticBag demo_diagnostics;
        const auto demo = load_transition_demo(transition, demo_diagnostics);
        if (!demo.has_value()) {
            print_diagnostics(demo_diagnostics, err);
            all_ok = false;
            continue;
        }
        if (!render_transition_demo(*demo, transition.shader_path, options.output_dir, demo_out, err)) {
            all_ok = false;
        }
    }

    if (options.json_output) {
        out << "{\"status\":\"" << (all_ok ? "ok" : "error") << "\",\"transitions\":" << transitions.size() << "}\n";
    }
    return all_ok;
}

} // namespace tachyon
