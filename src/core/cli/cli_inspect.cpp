#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/report.h"
#include "cli_internal.h"
#include <iostream>

namespace tachyon {

bool run_inspect_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    SceneSpec scene;
    AssetResolutionTable assets;
    if (!load_scene_context(options.scene_path, scene, assets, err)) return false;

    std::optional<RenderPlan> render_plan;
    std::optional<RenderExecutionPlan> execution_plan;
    if (!options.job_path.empty()) {
        const auto job_parsed = parse_render_job_file(options.job_path);
        if (!job_parsed.value.has_value()) {
            print_diagnostics(job_parsed.diagnostics, err);
            return false;
        }

        const auto plan_result = build_render_plan(scene, *job_parsed.value);
        if (!plan_result.value.has_value()) {
            print_diagnostics(plan_result.diagnostics, err);
            return false;
        }
        render_plan = *plan_result.value;

        const auto execution_result = build_render_execution_plan(*render_plan, assets.size());
        if (!execution_result.value.has_value()) {
            print_diagnostics(execution_result.diagnostics, err);
            return false;
        }
        execution_plan = *execution_result.value;
    }

    if (options.json_output) {
        out << make_inspect_report_json(scene, assets, render_plan, execution_plan) << '\n';
        return true;
    }

    print_inspect_report_text(scene, assets, render_plan, execution_plan, out);
    return true;
}

} // namespace tachyon
