#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "tachyon/core/spec/cpp_scene_loader.h"
#include "tachyon/scene/builder.h"
#include "tachyon/runtime/execution/native_render.h"
#include "tachyon/runtime/execution/batch/batch_runner.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/text/text_builders.h"
#include "cli_internal.h"
#include <iomanip>
#include <iostream>
#include <thread>
#include <string_view>

namespace tachyon {

namespace {
void print_execution_plan(
    const RenderExecutionPlan& execution_plan,
    const RasterizedFrame2D& rasterized_first_frame,
    const RenderSessionResult& session_result,
    std::ostream& out,
    std::size_t worker_count = 1) {
    out << "render execution plan valid\n";
    out << "composition: " << execution_plan.render_plan.composition_target << " (" << execution_plan.render_plan.composition.width << "x"
        << execution_plan.render_plan.composition.height << " @ " << execution_plan.render_plan.composition.frame_rate.value() << " fps, "
        << execution_plan.render_plan.composition.layer_count << " layers)\n";
    out << "frames: " << execution_plan.render_plan.frame_range.start << " -> " << execution_plan.render_plan.frame_range.end << '\n';

    // Phase timing table
    const double total_phase_time = session_result.scene_compile_ms + session_result.plan_build_ms 
                                 + session_result.execution_plan_build_ms + session_result.frame_execution_ms 
                                 + session_result.encode_ms;
    
    out << "\n  phase                      time        %\n";
    out << "  ─────────────────────────────────────────\n";
    constexpr std::size_t kPhaseColumnWidth = 27;
    constexpr std::size_t kTimeColumnWidth = 8;

    auto print_phase = [&](std::string_view name, double ms) {
        const double pct = total_phase_time > 0 ? (ms / total_phase_time) * 100.0 : 0.0;
        const std::size_t pad = name.size() < kPhaseColumnWidth ? (kPhaseColumnWidth - name.size()) : 1;
        out << "  " << name << std::string(pad, ' ')
            << std::fixed << std::setprecision(1) << std::setw(kTimeColumnWidth) << ms << "ms  "
            << std::fixed << std::setprecision(0) << std::setw(3) << pct << "%\n";
    };

    print_phase("scene compile",        session_result.scene_compile_ms);
    print_phase("plan build",           session_result.plan_build_ms);
    print_phase("execution plan build", session_result.execution_plan_build_ms);
    print_phase("frame execution",      session_result.frame_execution_ms);

    std::string encode_phase = "encode";
    if (!execution_plan.render_plan.output.profile.video.codec.empty())
        encode_phase += " (" + execution_plan.render_plan.output.profile.video.codec + ")";
    print_phase(encode_phase, session_result.encode_ms);

    out << "  ─────────────────────────────────────────\n";
    print_phase("total", total_phase_time);
    out << "\n";

    // Frame stats
    if (!session_result.frame_times_ms.empty()) {
        double sum = 0.0;
        double peak = 0.0;
        std::size_t peak_frame = 0;
        for (std::size_t i = 0; i < session_result.frame_times_ms.size(); ++i) {
            const double t = session_result.frame_times_ms[i];
            sum += t;
            if (t > peak) {
                peak = t;
                peak_frame = i;
            }
        }
        const double avg = sum / session_result.frame_times_ms.size();
        out << "  frames: " << session_result.frames_written << "  avg " << avg << "ms/frame  peak " 
            << peak << "ms (frame " << peak_frame << ")\n";
    }

    // Cache stats
    out << "  cache: " << session_result.cache_hits << " hits / " << session_result.cache_misses << " misses ("
        << static_cast<int>(session_result.cache_hit_rate()) << "%)\n";

    // Memory and workers
    if (session_result.peak_memory_bytes > 0) {
        out << "  memory: peak " << (session_result.peak_memory_bytes / (1024 * 1024)) << "MB\n";
    }
    out << "  workers: " << worker_count << " threads\n";
    out << "  2d runtime backend: " << rasterized_first_frame.backend_name << '\n';
}

bool build_scene_from_preset(const std::string& pid, SceneSpec& scene, std::ostream& err) {
    using namespace presets::background;
    
    LayerSpec bg;
    if (pid == "midnight_silk") bg = procedural_bg::midnight_silk(1280, 720);
    else if (pid == "golden_horizon") bg = procedural_bg::golden_horizon(1280, 720);
    else if (pid == "cyber_matrix") bg = procedural_bg::cyber_matrix(1280, 720);
    else if (pid == "frosted_glass") bg = procedural_bg::frosted_glass(1280, 720);
    else if (pid == "cosmic_nebula") bg = procedural_bg::cosmic_nebula(1280, 720);
    else if (pid == "brushed_metal") bg = procedural_bg::brushed_metal(1280, 720);
    else if (pid == "oceanic_abyss") bg = procedural_bg::oceanic_abyss(1280, 720);
    else if (pid == "royal_velvet") bg = procedural_bg::royal_velvet(1280, 720);
    else if (pid == "prismatic_light") bg = procedural_bg::prismatic_light(1280, 720);
    else if (pid == "technical_blueprint") bg = procedural_bg::technical_blueprint(1280, 720);
    else if (pid == "brushed_metal_title") {
        presets::TextParams tp;
        tp.text = "TACHYON NATIVE";
        tp.font_size = 120;
        tp.reveal_duration = 0.8;
        bg = presets::build_text_brushed_metal_title(tp);
    }
    else {
        err << "Unknown preset: " << pid << "\n";
        return false;
    }

    scene = ::tachyon::scene::Composition("preset_render")
        .size(1280, 720)
        .duration(2.0)
        .fps(30)
        .layer("bg", [&](::tachyon::scene::LayerBuilder& l) {
            l = ::tachyon::scene::LayerBuilder(bg);
        })
        .build_scene();
    return true;
}
}

bool run_render_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    if (!options.batch_path.empty()) {
        const auto parsed_batch = ::tachyon::parse_render_batch_file(options.batch_path);
        if (!parsed_batch.ok()) { print_diagnostics(parsed_batch.diagnostics, err); return false; }
        const auto batch_result = ::tachyon::run_render_batch(*parsed_batch.value, options.worker_count);
        if (!batch_result.value.has_value()) { print_diagnostics(batch_result.diagnostics, err); return false; }
        out << "Batch completed: " << batch_result.value->succeeded << " succeeded, " << batch_result.value->failed << " failed.\n";
        return batch_result.ok();
    }

    SceneSpec scene;
    AssetResolutionTable assets;
    
    if (!options.cpp_path.empty()) {
        if (!options.json_output) out << "[NativeLoader] Compiling scene from " << options.cpp_path.string() << "...\n";
        const auto result = CppSceneLoader::load_from_file(options.cpp_path);
        if (!result.success) {
            err << "C++ Scene Loader failed:\n" << result.diagnostics << "\n";
            return false;
        }
        scene = std::move(*result.scene);
    } else if (options.preset_id.has_value()) {
        if (!build_scene_from_preset(*options.preset_id, scene, err)) return false;
    } else if (!options.scene_path.empty()) {
        err << "WARNING: Rendering from JSON scene files is DEPRECATED. Use the new C++ Spec API (--cpp).\n";
        if (!load_scene_context(options.scene_path, scene, assets, err)) return false;
    } else {
        err << "Either --cpp, --scene or --preset must be provided.\n";
        return false;
    }

    RenderJob job;
    if (!options.job_path.empty()) {
        const auto job_parsed = parse_render_job_file(options.job_path);
        if (!job_parsed.value.has_value()) { print_diagnostics(job_parsed.diagnostics, err); return false; }
        job = *job_parsed.value;
    } else {
        // Default job if none provided
        job.job_id = "native_render";
        if (scene.compositions.empty()) {
            err << "Scene has no compositions.\n";
            return false;
        }
        job.composition_target = scene.compositions.front().id;
        job.frame_range = {0, static_cast<std::int64_t>(scene.compositions.front().duration * scene.compositions.front().frame_rate.value())};
        job.output.destination.path = !options.output_override.empty() ? options.output_override.string() : "output.mp4";
        job.output.profile.container = "mp4";
        job.output.profile.video.codec = "libx264";
        job.output.profile.video.pixel_format = "yuv420p";
    }

    if (!options.output_override.empty()) job.output.destination.path = options.output_override.string();
    if (options.frame_range_override.has_value()) job.frame_range = *options.frame_range_override;

    NativeRenderer::Options native_options;
    native_options.worker_count = options.worker_count;
    native_options.memory_budget_bytes = options.memory_budget_bytes;
    native_options.verbose = !options.json_output;

    const RenderSessionResult session_result = NativeRenderer::render(scene, job, native_options);
    
    if (!session_result.output_error.empty()) {
        err << "Render error: " << session_result.output_error << "\n";
    }

    print_diagnostics(session_result.diagnostics, err);

    if (options.json_output) {
        out << "{\"status\": \"ok\", \"frames\": " << session_result.frames.size() << "}\n";
    } else {
        RasterizedFrame2D first_frame;
        if (!session_result.frames.empty()) {
            first_frame.backend_name = "cpu-frame-executor";
        }
        
        // We need a RenderExecutionPlan for the execution plan printout
        // In a real refactor, NativeRenderer might return this or we build it here for the legacy printout
        const auto plan_result = build_render_plan(scene, job);
        if (plan_result.value.has_value()) {
            const auto execution_result = build_render_execution_plan(*plan_result.value, assets.size());
            if (execution_result.value.has_value()) {
                const std::size_t workers = (options.worker_count > 0) ? options.worker_count : std::thread::hardware_concurrency();
                print_execution_plan(*execution_result.value, first_frame, session_result, out, workers);
            }
        }
    }
    return session_result.output_error.empty();
}

bool run_preview_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    SceneSpec scene;
    AssetResolutionTable assets;

    if (!options.cpp_path.empty()) {
        const auto result = CppSceneLoader::load_from_file(options.cpp_path);
        if (!result.success) {
            err << "C++ Scene Loader failed:\n" << result.diagnostics << "\n";
            return false;
        }
        scene = std::move(*result.scene);
    } else if (options.preset_id.has_value()) {
        if (!build_scene_from_preset(*options.preset_id, scene, err)) return false;
    } else {
        if (!load_scene_context(options.scene_path, scene, assets, err)) return false;
    }

    std::string composition_id = scene.compositions.front().id;
    std::int64_t frame = options.preview_frame_number.has_value() ? *options.preview_frame_number : 0;
    std::filesystem::path output = !options.preview_output.empty() ? options.preview_output : "preview.png";

    if (!options.json_output) {
        out << "[NativePreview] Rendering frame " << frame << " of preset '" << (options.preset_id ? *options.preset_id : "custom") << "' to " << output.string() << "\n";
    }

    const bool success = NativeRenderer::render_still(scene, composition_id, frame, output);
    if (!success) {
        err << "Preview render failed.\n";
    } else if (options.json_output) {
        out << "{\"status\": \"ok\", \"output\": \"" << output.string() << "\"}\n";
    }

    return success;
}

} // namespace tachyon
