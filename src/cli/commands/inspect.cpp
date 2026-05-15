#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/assets/asset_resolution.h"
#include "tachyon/core/cli_scene_loader.h"
#include "tachyon/runtime/diagnostics/report.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/runtime/registry/engine_registry.h"
#include "tachyon/api.h"
#include "cli/commands/command.h"

#include <iostream>
#include <filesystem>
#include <optional>

namespace tachyon {

int run_inspect(const CliOptions& options, std::ostream& out, std::ostream& err) {
    out << "inspecting scene: " << options.cpp_path << "\n";

    SceneLoadOptions load_opts;
    load_opts.cpp_path = options.cpp_path;
    load_opts.preset_id = options.preset_id;

    auto loaded = load_scene_for_cli(load_opts, SceneLoadMode::Inspect, out, err);
    if (!loaded.success) {
        err << "failed to load scene for inspection.\n";
        for (const auto& d : loaded.diagnostics.diagnostics) {
            err << "  [" << diagnostic_severity_string(d.severity) << "] " << d.message << "\n";
        }
        return 1;
    }

    const SceneSpec& scene = loaded.context->scene;
    const core::assets::AssetResolutionTable& assets = loaded.context->assets;

    std::optional<RenderPlan> plan;
    std::optional<RenderExecutionPlan> execution_plan;

    // Try to build a dummy render plan for inspection if we have enough info
    if (!scene.compositions.empty()) {
        RenderJob job;
        job.composition_target = scene.compositions.front().id;
        job.frame_range = {0, static_cast<int>(scene.compositions.front().duration * 60.0)}; // Default 60fps estimate
        
        auto plan_res = build_render_plan(scene, job);
        if (plan_res.value.has_value()) {
            plan = std::move(*plan_res.value);
            
            auto exec_res = build_render_execution_plan(*plan, assets.size());
            if (exec_res.value.has_value()) {
                execution_plan = std::move(*exec_res.value);
            }
        }
    }

    print_inspect_report_text(scene, assets, plan, execution_plan, out);

    return 0;
}

bool run_inspect_command(const CliOptions& options, std::ostream& out, std::ostream& err, ::tachyon::runtime::EngineRegistry& /*bundle*/) {
    return run_inspect(options, out, err) == 0;
}

::tachyon::CommandDescriptor make_inspect_command() {
    return {
        "inspect",
        "tachyon inspect --cpp <path> [--preset <id>] [--json] [--info] [--samples <n>]",
        nullptr,
        run_inspect_command
    };
}

} // namespace tachyon
