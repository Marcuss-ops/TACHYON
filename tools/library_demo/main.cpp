#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/cpp_scene_loader.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/output/frame_output_sink.h"
#include "tachyon/renderer2d/effects/effect_host.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/effect_utils.h"
#include "tachyon/presets/effects/effect_manifest.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/core/transitions/basic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/artistic_transitions.h"
#include "tachyon/renderer2d/effects/core/transitions/light_leak_transitions.h"
#include "tachyon/core/library/library.h"
#include "tachyon/media/management/media_manager.h"
#include "tachyon/presets/text/text_registry.h"
#include "tachyon/presets/text/text_manifest.h"

#include <cmath>
#include <functional>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <map>
#include <vector>
#include <sstream>
#include <string_view>
#include <system_error>
#include <filesystem>

using namespace tachyon;

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

void scale_scene_preview(SceneSpec& scene, float scale) {
    if (scale <= 0.0f || scale >= 1.0f) return;
    for (auto& composition : scene.compositions) {
        composition.width = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::lround(static_cast<double>(composition.width) * scale)));
        composition.height = std::max<std::int64_t>(1, static_cast<std::int64_t>(std::lround(static_cast<double>(composition.height) * scale)));
        for (auto& layer : composition.layers) {
            layer.width = std::max(1, static_cast<int>(std::lround(static_cast<double>(layer.width) * scale)));
            layer.height = std::max(1, static_cast<int>(std::lround(static_cast<double>(layer.height) * scale)));
        }
    }
}

std::filesystem::path make_temp_still_path(const std::string& label) {
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / "tachyon_demo";
    std::error_code ec;
    std::filesystem::create_directories(temp_dir, ec);
    std::string stem = label;
    for (char& ch : stem) if (ch == ':' || ch == '\\' || ch == '/' || ch == ' ' || ch == '\t') ch = '_';
    if (stem.empty()) stem = "scene";
    return temp_dir / (stem + ".png");
}

std::optional<renderer2d::SurfaceRGBA> load_rendered_still(const std::filesystem::path& frame_path, std::ostream& err) {
    media::MediaManager media_manager;
    const auto* loaded = media_manager.get_image(frame_path, media::AlphaMode::Straight, nullptr);
    if (!loaded) {
        err << "failed to load rendered still: " << frame_path.string() << '\n';
        return std::nullopt;
    }
    return *loaded;
}

std::filesystem::path resolve_library_root(const std::filesystem::path& requested_root) {
    if (!requested_root.empty()) return std::filesystem::absolute(requested_root);
    const std::filesystem::path current = std::filesystem::current_path();
    const std::vector<std::filesystem::path> candidates = {
        current / "assets" / "library",
        current.parent_path() / "assets" / "library",
        current.parent_path().parent_path() / "assets" / "library"
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) return std::filesystem::absolute(candidate);
    }
    return std::filesystem::absolute(current / "assets" / "library");
}

std::optional<TransitionDemoConfig> load_transition_demo(const CliOptions& options, const LibraryTransitionEntry& entry, DiagnosticBag& diagnostics) {
    TransitionDemoConfig config;
    config.transition_id = entry.id;
    config.name = entry.name;
    config.source_scene_id = "tachyon.scene.a";
    config.target_scene_id = "tachyon.scene.b";
    config.output_dir = entry.output_dir;
    constexpr std::string_view prefix = "tachyon.transition.";
    config.file_prefix = entry.id.rfind(prefix, 0) == 0 ? entry.id.substr(prefix.size()) : entry.id;
    config.output_format = options.output_format.empty() ? "png" : options.output_format;
    config.duration_seconds = 0.5;
    config.lead_in_seconds = 0.5;
    config.lead_out_seconds = 0.5;
    config.progress_start = 0.0;
    config.progress_end = 1.0;
    config.preview_frame_count = 30;
    config.preview_resolution_scale = 1.0f;
    if (config.file_prefix.empty()) {
        diagnostics.add_error("library.demo_invalid", "Transition id is missing a file prefix.");
        return std::nullopt;
    }
    return config;
}

std::optional<renderer2d::SurfaceRGBA> render_scene_still(const SceneSpec& scene, const std::string& label, std::ostream& err, double& out_fps) {
    if (scene.compositions.empty()) {
        err << "scene has no compositions: " << label << '\n';
        return std::nullopt;
    }
    const CompositionSpec& composition = scene.compositions.front();
    out_fps = composition.frame_rate.denominator > 0 ? static_cast<double>(composition.frame_rate.numerator) / static_cast<double>(composition.frame_rate.denominator) : 30.0;
    if (out_fps <= 0.0) out_fps = 30.0;

    const std::filesystem::path temp_output = make_temp_still_path(label);
    TransitionRegistry transition_registry;
    register_builtin_transitions(transition_registry);
    for (auto* desc : transition_registry.list_all()) {
        renderer2d::resolve_basic_transition_implementations(const_cast<TransitionDescriptor&>(*desc));
        renderer2d::resolve_artistic_transition_implementations(const_cast<TransitionDescriptor&>(*desc));
        renderer2d::resolve_light_leak_implementations(const_cast<TransitionDescriptor&>(*desc));
    }
    presets::TextManifest text_manifest;
    presets::TextRegistry text_registry(text_manifest);
    if (!NativeRenderer::render_still(scene, composition.id, 0, temp_output, transition_registry, text_registry)) {
        err << "scene rendered no frames: " << label << '\n';
        return std::nullopt;
    }
    const std::filesystem::path frame_path = temp_output.parent_path() / (temp_output.stem().string() + "_000001.png");
    const auto loaded = load_rendered_still(frame_path, err);
    std::error_code ec;
    std::filesystem::remove(frame_path, ec);
    return loaded;
}

std::optional<std::reference_wrapper<const CachedSceneStill>> render_scene_still_cached(const std::string& scene_id, const TachyonLibrary& library, std::map<std::string, CachedSceneStill>& cache, std::ostream& err) {
    const auto cache_it = cache.find(scene_id);
    if (cache_it != cache.end()) return std::cref(cache_it->second);
    double fps = 30.0;
    SceneSpec scene = library.instantiate_scene(scene_id);
    scale_scene_preview(scene, 0.35f);
    if (scene.compositions.empty()) {
        err << "unknown or empty library scene: " << scene_id << '\n';
        return std::nullopt;
    }
    const auto frame = render_scene_still(scene, scene_id, err, fps);
    if (!frame.has_value()) return std::nullopt;
    auto [insert_it, inserted] = cache.emplace(scene_id, CachedSceneStill{*frame, fps});
    return std::cref(insert_it->second);
}

renderer2d::SurfaceRGBA resize_surface(const renderer2d::SurfaceRGBA& src, std::uint32_t width, std::uint32_t height) {
    if (src.width() == width && src.height() == height) return src;
    renderer2d::SurfaceRGBA out(width, height);
    out.set_profile(src.profile());
    if (width == 0U || height == 0U) return out;
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
    plan.job_id = "library-demo";
    plan.scene_ref = "assets/library";
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

bool render_transition_demo(const TransitionDemoConfig& demo, const renderer2d::SurfaceRGBA& source, const renderer2d::SurfaceRGBA& target, const std::filesystem::path& transition_shader_path, const std::filesystem::path& output_dir_override, std::ostream& out, std::ostream& err) {
    const bool final_video = demo.output_format == "mp4" || demo.output_format == "mov" || demo.output_format == "mkv" || demo.output_format == "webm";
    const float render_scale = final_video ? 1.0f : demo.preview_resolution_scale;
    const renderer2d::SurfaceRGBA preview_source = preview_surface(source, render_scale);
    const renderer2d::SurfaceRGBA preview_target = preview_surface(target, render_scale);
    const double fps = final_video ? 24.0 : 30.0;
    const double total_duration_seconds = demo.lead_in_seconds + demo.duration_seconds + demo.lead_out_seconds;

    renderer2d::EffectRegistry registry;
    TransitionRegistry transition_registry;
    register_builtin_transitions(transition_registry);
    for (auto* desc : transition_registry.list_all()) {
        renderer2d::resolve_basic_transition_implementations(const_cast<TransitionDescriptor&>(*desc));
        renderer2d::resolve_artistic_transition_implementations(const_cast<TransitionDescriptor&>(*desc));
        renderer2d::resolve_light_leak_implementations(const_cast<TransitionDescriptor&>(*desc));
    }
    presets::EffectManifest effect_manifest;
    renderer2d::register_builtin_effects(registry, effect_manifest, transition_registry);
    auto host = renderer2d::create_effect_host(registry);

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
        output_plan.composition.frame_rate = {24, 1};
        output_plan.composition.duration = total_duration_seconds;
    }
    auto sink = output::create_frame_output_sink(output_plan);
    if (!sink) {
        err << "failed to create sink for " << demo.output_dir.string() << '\n';
        return false;
    }
    if (!sink->begin(output_plan)) {
        err << sink->last_error() << '\n';
        return false;
    }
    const std::size_t frame_count = final_video ? std::max<std::size_t>(2, static_cast<std::size_t>(std::llround(total_duration_seconds * fps))) : std::max<std::size_t>(2, demo.preview_frame_count);
    const double progress_span = demo.progress_end - demo.progress_start;
    const std::size_t lead_in_frames = std::max<std::size_t>(1, static_cast<std::size_t>(std::floor(demo.lead_in_seconds * fps)));
    const std::size_t lead_out_frames = std::max<std::size_t>(1, static_cast<std::size_t>(std::floor(demo.lead_out_seconds * fps)));
    const std::size_t transition_frames = std::max<std::size_t>(2, frame_count - lead_in_frames - lead_out_frames);
    std::size_t written = 0;
    for (std::size_t index = 0; index < frame_count; ++index) {
        float t = 0.0f;
        if (index < lead_in_frames) t = 0.0f;
        else if (index < lead_in_frames + transition_frames) {
            const std::size_t local_index = index - lead_in_frames;
            const double normalized = transition_frames == 1 ? 1.0 : static_cast<double>(local_index) / static_cast<double>(transition_frames - 1);
            t = static_cast<float>(demo.progress_start + progress_span * normalized);
        } else t = 1.0f;
        renderer2d::EffectParams params;
        params.scalars["t"] = t;
        params.scalars["duration_seconds"] = static_cast<float>(demo.duration_seconds);
        params.strings["transition_id"] = demo.transition_id;
        params.strings["shader_path"] = transition_shader_path.generic_string();
        params.aux_surfaces["transition_to"] = &preview_target;
        const auto blended_result = host->apply("tachyon.effect.transition.glsl", preview_source, params);
        const renderer2d::SurfaceRGBA output_frame = final_video ? resize_surface(blended_result.value.value_or(preview_source), static_cast<std::uint32_t>(source.width()), static_cast<std::uint32_t>(source.height())) : blended_result.value.value_or(preview_source);
        output::OutputFramePacket packet;
        packet.frame_number = static_cast<std::int64_t>(index + 1);
        packet.frame = &output_frame;
        packet.metadata.time_seconds = static_cast<double>(index) / fps;
        packet.metadata.timecode = std::to_string(index + 1);
        packet.metadata.scene_hash = demo.transition_id;
        if (!sink->write_frame(packet)) { err << sink->last_error() << '\n'; return false; }
        ++written;
    }
    if (!sink->finish()) { err << sink->last_error() << '\n'; return false; }
    out << demo.transition_id << ": wrote " << written << " frame(s) to " << final_output_dir.generic_string() << '\n';
    return true;
}

} // namespace

int main(int argc, char** argv) {
    const auto parsed = parse_cli_options(argc, argv);
    if (!parsed.value.has_value()) {
        std::cerr << "failed to parse options\n";
        return 1;
    }
    const CliOptions& options = *parsed.value;

    const std::filesystem::path library_root = resolve_library_root(options.library_path);
    TachyonLibrary library(library_root);
    if (!library.ok()) {
        DiagnosticBag diagnostics = library.diagnostics();
        for (const auto& d : diagnostics.list()) std::cerr << d.message << '\n';
        return 1;
    }

    std::vector<LibraryTransitionEntry> transitions;
    if (options.transition_id.has_value()) {
        const auto transition = library.find_transition(*options.transition_id);
        if (!transition.has_value()) {
            std::cerr << "transition not found: " << *options.transition_id << '\n';
            return 1;
        }
        transitions.push_back(*transition);
    } else {
        transitions = library.transitions();
    }

    if (transitions.empty()) {
        std::cerr << "no transitions available in " << library_root.generic_string() << '\n';
        return 1;
    }

    std::map<std::string, CachedSceneStill> scene_still_cache;
    bool all_ok = true;
    for (const auto& transition : transitions) {
        DiagnosticBag demo_diagnostics;
        const auto demo = load_transition_demo(options, transition, demo_diagnostics);
        if (!demo.has_value()) {
            for (const auto& d : demo_diagnostics.list()) std::cerr << d.message << '\n';
            all_ok = false;
            continue;
        }

        const auto source_still = render_scene_still_cached(demo->source_scene_id, library, scene_still_cache, std::cerr);
        if (!source_still.has_value()) { all_ok = false; continue; }
        const auto target_still = render_scene_still_cached(demo->target_scene_id, library, scene_still_cache, std::cerr);
        if (!target_still.has_value()) { all_ok = false; continue; }

        const renderer2d::SurfaceRGBA resized_target = resize_surface(target_still->get().frame, source_still->get().frame.width(), source_still->get().frame.height());

        if (!render_transition_demo(*demo, source_still->get().frame, resized_target, transition.shader_path, options.output_dir, std::cout, std::cerr)) {
            all_ok = false;
        }
    }

    return all_ok ? 0 : 1;
}
