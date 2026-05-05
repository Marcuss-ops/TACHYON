#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/cpp_scene_loader.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/effects/effect_utils.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/core/catalog/catalog.h"
#include "cli_internal.h"

#include <cmath>
#include <functional>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <vector>
#include <sstream>
#include <string_view>

namespace tachyon {
namespace {

struct TransitionDemoConfig {
    std::string transition_id;
    std::string name;
    std::string source_scene_id;
    std::string target_scene_id;
    std::filesystem::path output_dir;
    std::string file_prefix;
    std::string output_format{"mp4"};
    double duration_seconds{0.2};
    double lead_in_seconds{0.9};
    double lead_out_seconds{0.9};
    double progress_start{0.0};
    double progress_end{1.0};
    std::size_t preview_frame_count{12};
    float preview_resolution_scale{1.0f};
};

struct CachedSceneStill {
    renderer2d::SurfaceRGBA frame;
    double fps{30.0};
};

std::filesystem::path resolve_catalog_root(const std::filesystem::path& requested_root) {
    if (!requested_root.empty()) {
        return std::filesystem::absolute(requested_root);
    }

    const std::filesystem::path current = std::filesystem::current_path();
    const std::vector<std::filesystem::path> candidates = {
        current / "assets" / "catalog",
        current.parent_path() / "assets" / "catalog",
        current.parent_path().parent_path() / "assets" / "catalog"
    };

    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return std::filesystem::absolute(candidate);
        }
    }

    return std::filesystem::absolute(current / "assets" / "catalog");
}

std::optional<TransitionDemoConfig> load_transition_demo(
    const CatalogTransitionEntry& entry,
    DiagnosticBag& diagnostics) {
    TransitionDemoConfig config;
    config.transition_id = entry.id;
    config.name = entry.name;
    config.source_scene_id = "tachyon.scene.a";
    config.target_scene_id = "tachyon.scene.b";
    config.output_dir = entry.output_dir;
    constexpr std::string_view prefix = "tachyon.transition.";
    config.file_prefix = entry.id.rfind(prefix, 0) == 0 ? entry.id.substr(prefix.size()) : entry.id;
    config.output_format = "mp4";
    config.duration_seconds = 0.5;
    config.lead_in_seconds = 1.25;
    config.lead_out_seconds = 1.25;
    config.progress_start = 0.0;
    config.progress_end = 1.0;
    config.preview_frame_count = 12;
    config.preview_resolution_scale = 1.0f;

    if (config.file_prefix.empty()) {
        diagnostics.add_error("catalog.demo_invalid", "Transition id is missing a file prefix.");
        return std::nullopt;
    }

    return config;
}

std::optional<renderer2d::SurfaceRGBA> render_scene_still(
    const std::filesystem::path& scene_path,
    std::ostream& err,
    double& out_fps) {
    if (scene_path.extension() != ".cpp") {
        err << "Error: Only C++ scenes (.cpp) are supported.\n";
        return std::nullopt;
    }

    SceneSpec scene;
    const auto cpp_result = CppSceneLoader::load_from_file(scene_path);
    if (!cpp_result.success) {
        err << "C++ Scene Loader failed for " << scene_path.string() << ":\n" << cpp_result.diagnostics << "\n";
        return std::nullopt;
    }
    scene = std::move(*cpp_result.scene);

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

std::optional<renderer2d::SurfaceRGBA> render_scene_still(
    const SceneSpec& scene,
    const std::string& label,
    std::ostream& err,
    double& out_fps) {
    if (scene.compositions.empty()) {
        err << "scene has no compositions: " << label << '\n';
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
    job.job_id = scene.project.id.empty() ? label + "_demo" : scene.project.id + "_demo";
    job.scene_ref = label;
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
        err << "scene rendered no frames: " << label << '\n';
        return std::nullopt;
    }

    return *session_result.frames.front().frame;
}

std::optional<std::reference_wrapper<const CachedSceneStill>> render_scene_still_cached(
    const std::string& scene_id,
    const TachyonCatalog& catalog,
    std::map<std::string, CachedSceneStill>& cache,
    std::ostream& err) {
    const auto cache_it = cache.find(scene_id);
    if (cache_it != cache.end()) {
        return std::cref(cache_it->second);
    }

    double fps = 30.0;
    SceneSpec scene = catalog.instantiate_scene(scene_id);

    // Keep the legacy image-based demo path only when the asset actually exists.
    // Legacy support for older demo asset names.
    if (scene.compositions.empty() && (scene_id == "scene_a" || scene_id == "scene_b")) {
        const std::string filename = scene_id + ".png";
        const std::filesystem::path asset_path = catalog.root() / "assets" / filename;
        if (std::filesystem::exists(asset_path)) {
            AssetSpec asset;
            asset.id = scene_id + "_asset";
            asset.path = asset_path.generic_string();
            asset.type = "image";
            scene.assets.push_back(asset);

            CompositionSpec comp;
            comp.id = "main";
            comp.width = 1920;
            comp.height = 1080;
            comp.duration = 1.0;
            comp.frame_rate = {30, 1};

            LayerSpec layer;
            layer.id = "image";
            layer.type = "image";
            layer.asset_id = asset.id;
            layer.width = 1920;
            layer.height = 1080;
            comp.layers.push_back(layer);
            scene.compositions.push_back(comp);
        }
    }

    if (scene.compositions.empty()) {
        err << "unknown or empty catalog scene: " << scene_id << '\n';
        return std::nullopt;
    }

    const auto frame = render_scene_still(scene, scene_id, err, fps);
    if (!frame.has_value()) {
        return std::nullopt;
    }

    auto [insert_it, inserted] = cache.emplace(scene_id, CachedSceneStill{*frame, fps});
    (void)inserted;
    return std::cref(insert_it->second);
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
    plan.job_id = "catalog-demo";
    plan.scene_ref = "assets/catalog";
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
    const renderer2d::SurfaceRGBA& source,
    const renderer2d::SurfaceRGBA& target,
    const std::filesystem::path& transition_shader_path,
    const std::filesystem::path& output_dir_override,
    std::ostream& out,
    std::ostream& err) {
    const bool final_video = demo.output_format == "mp4" || demo.output_format == "mov" || demo.output_format == "mkv" || demo.output_format == "webm";
    const float render_scale = final_video ? 1.0f : demo.preview_resolution_scale;
    const renderer2d::SurfaceRGBA preview_source = preview_surface(source, render_scale);
    const renderer2d::SurfaceRGBA preview_target = preview_surface(target, render_scale);
    const double fps = final_video ? 20.0 : 30.0;
    const double total_duration_seconds = demo.lead_in_seconds + demo.duration_seconds + demo.lead_out_seconds;

    auto host = renderer2d::create_effect_host();
    renderer2d::EffectHost::register_builtins(*host);

    const std::filesystem::path final_output_dir = output_dir_override.empty() ? demo.output_dir : output_dir_override;
    RenderPlan output_plan = make_output_plan(final_output_dir, demo.file_prefix);
    output_plan.composition.width = static_cast<std::int64_t>(final_video ? source.width() : preview_source.width());
    output_plan.composition.height = static_cast<std::int64_t>(final_video ? source.height() : preview_source.height());
    if (final_video) {
        output_plan.output.destination.path = (final_output_dir / (demo.file_prefix + ".mp4")).generic_string();
        output_plan.output.profile.class_name = "video";
        output_plan.output.profile.container = "mp4";
        output_plan.output.profile.video.codec = "libx264";
        output_plan.output.profile.video.pixel_format = "yuv420p";
        output_plan.output.profile.video.rate_control_mode = "fixed";
        output_plan.output.profile.alpha_mode = "opaque";
        output_plan.composition.frame_rate = {20, 1};
        output_plan.composition.duration = total_duration_seconds;
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
        ? std::max<std::size_t>(2, static_cast<std::size_t>(std::llround(total_duration_seconds * fps)))
        : std::max<std::size_t>(2, demo.preview_frame_count);
    const double progress_span = demo.progress_end - demo.progress_start;
    const std::size_t lead_in_frames = std::max<std::size_t>(1, static_cast<std::size_t>(std::floor(demo.lead_in_seconds * fps)));
    const std::size_t lead_out_frames = std::max<std::size_t>(1, static_cast<std::size_t>(std::floor(demo.lead_out_seconds * fps)));
    const std::size_t transition_frames = std::max<std::size_t>(2, frame_count - lead_in_frames - lead_out_frames);
    std::size_t written = 0;

    for (std::size_t index = 0; index < frame_count; ++index) {
        float t = 0.0f;
        if (index < lead_in_frames) {
            t = 0.0f;
        } else if (index < lead_in_frames + transition_frames) {
            const std::size_t local_index = index - lead_in_frames;
            const double normalized = transition_frames == 1
                ? 1.0
                : static_cast<double>(local_index) / static_cast<double>(transition_frames - 1);
            t = static_cast<float>(demo.progress_start + progress_span * normalized);
        } else if (index < lead_in_frames + transition_frames + lead_out_frames) {
            t = 1.0f;
        } else {
            t = 1.0f;
        }

        renderer2d::EffectParams params;
        params.scalars["t"] = t;
        params.scalars["duration_seconds"] = static_cast<float>(demo.duration_seconds);
        params.strings["transition_id"] = demo.transition_id;
        params.strings["shader_path"] = transition_shader_path.generic_string();
        params.aux_surfaces["transition_to"] = &preview_target;

        const auto blended_result = host->apply("tachyon.effect.transition.glsl", preview_source, params);
        const renderer2d::SurfaceRGBA output_frame = final_video
            ? resize_surface(blended_result.value.value_or(preview_source), static_cast<std::uint32_t>(source.width()), static_cast<std::uint32_t>(source.height()))
            : blended_result.value.value_or(preview_source);

        output::OutputFramePacket packet;
        packet.frame_number = static_cast<std::int64_t>(index + 1);
        packet.frame = &output_frame;
        packet.metadata.time_seconds = static_cast<double>(index) / fps;
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

bool run_catalog_demo_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    const std::filesystem::path catalog_root = resolve_catalog_root(options.catalog_path);
    TachyonCatalog catalog(catalog_root);
    if (!catalog.ok()) {
        print_diagnostics(catalog.diagnostics(), err);
        return false;
    }

    std::vector<CatalogTransitionEntry> transitions;
    if (options.transition_id.has_value()) {
        const auto transition = catalog.find_transition(*options.transition_id);
        if (!transition.has_value()) {
            err << "transition not found: " << *options.transition_id << '\n';
            return false;
        }
        transitions.push_back(*transition);
    } else {
        transitions = catalog.transitions();
    }

    if (transitions.empty()) {
        err << "no transitions available in " << catalog_root.generic_string() << '\n';
        return false;
    }

    std::ostream& demo_out = out;
    std::map<std::string, CachedSceneStill> scene_still_cache;

    bool all_ok = true;
    for (const auto& transition : transitions) {
        DiagnosticBag demo_diagnostics;
        const auto demo = load_transition_demo(transition, demo_diagnostics);
        if (!demo.has_value()) {
            print_diagnostics(demo_diagnostics, err);
            all_ok = false;
            continue;
        }

        const auto source_still = render_scene_still_cached(demo->source_scene_id, catalog, scene_still_cache, err);
        if (!source_still.has_value()) {
            all_ok = false;
            continue;
        }
        const auto target_still = render_scene_still_cached(demo->target_scene_id, catalog, scene_still_cache, err);
        if (!target_still.has_value()) {
            all_ok = false;
            continue;
        }

        const renderer2d::SurfaceRGBA resized_target = resize_surface(
            target_still->get().frame,
            source_still->get().frame.width(),
            source_still->get().frame.height());

        if (!render_transition_demo(*demo, source_still->get().frame, resized_target, transition.shader_path, options.output_dir, demo_out, err)) {
            all_ok = false;
        }
    }

    return all_ok;
}

} // namespace tachyon
