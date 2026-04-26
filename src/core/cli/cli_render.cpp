#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/execution/batch/batch_runner.h"
#include "tachyon/core/spec/compilation/scene_compiler.h"
#include "cli_internal.h"
#include <filesystem>
#include <iostream>
#include <thread>

namespace tachyon {

namespace {
void print_execution_plan(
    const RenderExecutionPlan& execution_plan,
    const RasterizedFrame2D& rasterized_first_frame,
    const RenderSessionResult& session_result,
    std::ostream& out) {
    out << "render execution plan valid\n";
    out << "scene: " << execution_plan.render_plan.scene_ref << '\n';
    out << "job: " << execution_plan.render_plan.job_id << '\n';
    out << "composition: " << execution_plan.render_plan.composition_target << " (" << execution_plan.render_plan.composition.width << "x"
        << execution_plan.render_plan.composition.height << " @ " << execution_plan.render_plan.composition.frame_rate.value() << " fps, "
        << execution_plan.render_plan.composition.layer_count << " layers)\n";
    out << "frames: " << execution_plan.render_plan.frame_range.start << " -> " << execution_plan.render_plan.frame_range.end << '\n';
    out << "resolved assets: " << execution_plan.resolved_asset_count << '\n';
    out << "rendered frames: " << session_result.frames.size() << '\n';
    out << "written frames: " << session_result.frames_written << '\n';
    out << "cache hits: " << session_result.cache_hits << '\n';
    out << "cache misses: " << session_result.cache_misses << '\n';
    out << "2d runtime backend: " << rasterized_first_frame.backend_name << '\n';
}
}

bool run_render_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    if (!options.batch_path.empty()) {
        const auto parsed_batch = parse_render_batch_file(options.batch_path);
        if (!parsed_batch.ok()) { print_diagnostics(parsed_batch.diagnostics, err); return false; }
        const auto batch_result = run_render_batch(*parsed_batch.value, options.worker_count);
        if (!batch_result.value.has_value()) { print_diagnostics(batch_result.diagnostics, err); return false; }
        out << "Batch completed: " << batch_result.value->succeeded << " succeeded, " << batch_result.value->failed << " failed.\n";
        return batch_result.ok();
    }

    SceneSpec scene;
    AssetResolutionTable assets;
    if (!load_scene_context(options.scene_path, scene, assets, err)) return false;

    const auto job_parsed = parse_render_job_file(options.job_path);
    if (!job_parsed.value.has_value()) { print_diagnostics(job_parsed.diagnostics, err); return false; }
    RenderJob job = *job_parsed.value;

    if (!options.output_override.empty()) job.output.destination.path = options.output_override.string();
    if (options.frame_range_override.has_value()) job.frame_range = *options.frame_range_override;
    if (options.output_override.empty() && !options.output_dir.empty()) {
        std::filesystem::create_directories(options.output_dir);

        std::filesystem::path resolved_output_path = job.output.destination.path.empty()
            ? std::filesystem::path()
            : std::filesystem::path(job.output.destination.path);

        if (resolved_output_path.empty()) {
            std::filesystem::path filename = !job.job_id.empty()
                ? std::filesystem::path(job.job_id)
                : std::filesystem::path(!job.composition_target.empty() ? job.composition_target : "render");
            const std::string extension = !job.output.profile.container.empty()
                ? "." + job.output.profile.container
                : ".mp4";
            filename.replace_extension(extension);
            resolved_output_path = options.output_dir / filename.filename();
        } else {
            resolved_output_path = options.output_dir / resolved_output_path.filename();
        }

        job.output.destination.path = resolved_output_path.generic_string();
    }

    const auto plan_result = build_render_plan(scene, job);
    if (!plan_result.value.has_value()) { print_diagnostics(plan_result.diagnostics, err); return false; }
    const auto execution_result = build_render_execution_plan(*plan_result.value, assets.size());
    if (!execution_result.value.has_value()) { print_diagnostics(execution_result.diagnostics, err); return false; }

    SceneCompiler compiler;
    const auto compiled_result = compiler.compile(scene);
    if (!compiled_result.ok()) { print_diagnostics(compiled_result.diagnostics, err); return false; }

    RenderSession session;
    if (options.memory_budget_bytes.has_value()) session.set_memory_budget_bytes(*options.memory_budget_bytes);
    
    const std::size_t concurrency = (options.worker_count > 0) ? options.worker_count : std::thread::hardware_concurrency();
    const RenderSessionResult session_result = session.render(scene, *compiled_result.value, *execution_result.value, job.output.destination.path, concurrency);
    
    if (options.json_output) {
        // Reduced JSON output for brevity in this example
        out << "{\"status\": \"ok\", \"frames\": " << session_result.frames.size() << "}\n";
    } else {
        RasterizedFrame2D first_frame;
        if (!session_result.frames.empty()) {
            first_frame.backend_name = "cpu-frame-executor";
        }
        print_execution_plan(*execution_result.value, first_frame, session_result, out);
    }
    return session_result.output_error.empty();
}

} // namespace tachyon
