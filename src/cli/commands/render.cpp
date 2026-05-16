#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/assets/asset_resolution.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/runtime/compiler/scene_compiler.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/runtime/registry/engine_registry.h"
#include "tachyon/api.h"
#include "cli/commands/command.h"
#include "cli/support/cli_internal.h"
#include "tachyon/core/render_telemetry.h"
#include "tachyon/diagnostics/trace.h"

#include <iostream>
#include <filesystem>
#include <optional>
#include <chrono>

namespace tachyon {

int run_render(const CliOptions& options, std::ostream& out, std::ostream& err) {
    TACHYON_TRACE_SCOPE("render.total");
    RenderTelemetry::get().init();
    out << "rendering scene: " << options.cpp_path << "\n";

    SceneLoadOptions load_opts;
    load_opts.cpp_path = options.cpp_path;
    load_opts.preset_id = options.preset_id;

    auto loaded = load_scene_for_cli(load_opts, SceneLoadMode::Render, out, err);
    if (!loaded.success) {
        err << "failed to load scene for rendering.\n";
        for (const auto& d : loaded.diagnostics.diagnostics) {
            err << "  [" << diagnostic_severity_string(d.severity) << "] " << d.message << "\n";
        }
        return 1;
    }

    const SceneSpec& scene = loaded.context->scene;
    const core::assets::AssetResolutionTable& assets = loaded.context->assets;

    if (scene.compositions.empty()) {
        err << "error: scene contains no compositions.\n";
        return 1;
    }
    
    // For now, always render the first composition since the CLI doesn't have a flag to select it
    std::string comp_id = scene.compositions.front().id;

    RenderJob job;
    job.composition_target = comp_id;
    
    // Set frame range from options if provided, otherwise use composition duration
    if (options.render.frame_range_override.has_value()) {
        job.frame_range = *options.render.frame_range_override;
    } else {
        // Default to whole composition at 60fps
        job.frame_range = {0, static_cast<int>(scene.compositions.front().duration * 60.0)};
    }

    if (!options.render.output_override.empty()) {
        job.output.destination.path = options.render.output_override.string();
    } else if (!options.output_dir.empty()) {
        job.output.destination.path = (options.output_dir / (comp_id + ".mp4")).string();
    }
    
    // Build plan
    auto plan_res = build_render_plan(scene, job);
    if (!plan_res.value.has_value()) {
        err << "failed to build render plan.\n";
        return 1;
    }

    // Build execution plan
    auto exec_res = build_render_execution_plan(*plan_res.value, assets.size());
    if (!exec_res.value.has_value()) {
        err << "failed to build execution plan.\n";
        return 1;
    }

    // Compile scene
    SceneCompiler compiler;
    auto compiled = compiler.compile(scene);
    if (!compiled.ok()) {
        err << "failed to compile scene.\n";
        return 1;
    }

    // Render session
    RenderSession session;
    const std::filesystem::path output_path = job.output.destination.path.empty() ? std::filesystem::path{} : std::filesystem::path(job.output.destination.path);
    
    out << "starting render: " << (job.frame_range.end - job.frame_range.start) << " frames\n";
    auto start_time = std::chrono::high_resolution_clock::now();

    // Call with 6 arguments to avoid ambiguity in RenderSession::render overloads
    auto session_res = session.render(scene, *compiled.value, *exec_res.value, output_path, ::tachyon::runtime::RenderWorkerBudget{}, nullptr);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

    if (session_res.output_error.empty()) {
        out << "render completed successfully in " << (duration.count() / 1000.0) << "s\n";
        if (!output_path.empty()) {
            out << "output: " << output_path.string() << "\n";
        }

        TelemetryEvent e;
        e.event = "video_export";
        e.total_ms = static_cast<double>(duration.count());
        e.w = scene.compositions.front().width;
        e.h = scene.compositions.front().height;
        e.layer_count = static_cast<int>(scene.compositions.front().layers.size());
        RenderTelemetry::get().log(e);
        RenderTelemetry::get().save_summary();

        return 0;
    } else {
        err << "render failed: " << session_res.output_error << "\n";
        return 1;
    }
}

bool run_render_command(const CliOptions& options, std::ostream& out, std::ostream& err, ::tachyon::runtime::EngineRegistry& /*bundle*/) {
    return run_render(options, out, err) == 0;
}

::tachyon::CommandDescriptor make_render_command() {
    return {
        "render",
        "tachyon render --cpp <path> [--preset <id>] [--out <path>] [--frames <range>] [--workers <n>]",
        nullptr,
        run_render_command
    };
}

bool run_preview_command(const CliOptions& options, std::ostream& out, std::ostream& err, ::tachyon::runtime::EngineRegistry& bundle) {
    return run_preview_internal(options, out, err, "Preview", bundle);
}

::tachyon::CommandDescriptor make_preview_command() {
    return {
        "preview",
        "tachyon preview --cpp <path> [--preset <id>] [--frame <n>] [--out <path>]",
        nullptr,
        run_preview_command
    };
}

} // namespace tachyon
