#include "tachyon/core/cli.h"

#include "tachyon/core/cli_options.h"
#include "tachyon/core/core.h"
#include "tachyon/core/report.h"
#include "tachyon/media/asset_resolution.h"
#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/runtime/render_graph.h"
#include "tachyon/runtime/render_plan.h"
#include "tachyon/runtime/render_job.h"
#include "tachyon/spec/scene_spec.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

namespace tachyon {
namespace {

struct SceneContext {
    SceneSpec scene;
    AssetResolutionTable assets;
};

void print_diagnostics(const DiagnosticBag& diagnostics, std::ostream& out) {
    for (const auto& diagnostic : diagnostics.diagnostics) {
        out << diagnostic.code << ": " << diagnostic.message;
        if (!diagnostic.path.empty()) {
            out << " (" << diagnostic.path << ")";
        }
        out << '\n';
    }
}

std::filesystem::path scene_asset_root(const std::filesystem::path& scene_path) {
    const auto scene_dir = scene_path.parent_path();
    if (scene_dir.empty()) {
        return scene_dir;
    }

    const auto folder_name = scene_dir.filename().string();
    if ((folder_name == "scenes" || folder_name == "project") && !scene_dir.parent_path().empty()) {
        return scene_dir.parent_path();
    }

    return scene_dir;
}

void print_help(std::ostream& out) {
    out << "TACHYON " << version_string() << '\n';
    out << "Usage:\n";
    out << "  tachyon version\n";
    out << "  tachyon validate --scene <file> [--job <file>] [--json]\n";
    out << "  tachyon inspect --scene <file> [--job <file>] [--json]\n";
    out << "  tachyon render --scene <file> --job <file> [--out <file>] [--json]\n";
}

bool load_scene_context(const std::filesystem::path& scene_path, SceneContext& context, std::ostream& err) {
    const auto parsed = parse_scene_spec_file(scene_path);
    if (!parsed.value.has_value()) {
        print_diagnostics(parsed.diagnostics, err);
        return false;
    }

    const auto validation = validate_scene_spec(*parsed.value);
    if (!validation.ok()) {
        print_diagnostics(validation.diagnostics, err);
        return false;
    }

    const auto assets = resolve_assets(*parsed.value, scene_asset_root(scene_path));
    if (!assets.value.has_value()) {
        print_diagnostics(assets.diagnostics, err);
        return false;
    }

    context.scene = *parsed.value;
    context.assets = *assets.value;
    return true;
}

bool load_render_job(const std::filesystem::path& job_path, RenderJob& job, std::ostream& err) {
    const auto parsed = parse_render_job_file(job_path);
    if (!parsed.value.has_value()) {
        print_diagnostics(parsed.diagnostics, err);
        return false;
    }

    const auto validation = validate_render_job(*parsed.value);
    if (!validation.ok()) {
        print_diagnostics(validation.diagnostics, err);
        return false;
    }

    job = *parsed.value;
    return true;
}

void print_execution_plan(const RenderExecutionPlan& execution_plan, const RasterizedFrame2D& rasterized_first_frame, std::ostream& out) {
    out << "render execution plan valid\n";
    out << "scene: " << execution_plan.render_plan.scene_ref << '\n';
    out << "job: " << execution_plan.render_plan.job_id << '\n';
    out << "composition: " << execution_plan.render_plan.composition_target << " (" << execution_plan.render_plan.composition.width << "x"
        << execution_plan.render_plan.composition.height << " @ " << execution_plan.render_plan.composition.frame_rate.value() << " fps, "
        << execution_plan.render_plan.composition.layer_count << " layers)\n";
    out << "frames: " << execution_plan.render_plan.frame_range.start << " -> " << execution_plan.render_plan.frame_range.end << '\n';
    out << "resolved assets: " << execution_plan.resolved_asset_count << '\n';
    out << "graph steps: " << execution_plan.steps.size() << '\n';
    out << "frame tasks: " << execution_plan.frame_tasks.size() << '\n';
    out << "first frame cache key: " << rasterized_first_frame.cache_key << '\n';
    out << "2d refined backend: " << rasterized_first_frame.backend_name << '\n';
    out << "2d stub draw ops: " << rasterized_first_frame.estimated_draw_ops << '\n';
    out << "note: pixel rendering is not wired yet; this command validates the graph and stub raster slice\n";
}

bool run_validate_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    SceneContext context;
    if (!load_scene_context(options.scene_path, context, err)) {
        return false;
    }

    std::optional<RenderJob> job;
    bool job_valid = false;
    if (!options.job_path.empty()) {
        RenderJob loaded_job;
        if (!load_render_job(options.job_path, loaded_job, err)) {
            return false;
        }
        job = std::move(loaded_job);
        job_valid = true;
    }

    if (options.json_output) {
        out << make_validate_report_json(context.scene, context.assets, true, job_valid, job) << '\n';
        return true;
    }

    out << "scene spec valid\n";
    out << "resolved assets: " << context.assets.size() << '\n';
    if (job_valid) {
        out << "render job valid\n";
    }
    return true;
}

bool run_inspect_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    SceneContext context;
    if (!load_scene_context(options.scene_path, context, err)) {
        return false;
    }

    std::optional<RenderPlan> render_plan;
    std::optional<RenderExecutionPlan> execution_plan;
    if (!options.job_path.empty()) {
        RenderJob job;
        if (!load_render_job(options.job_path, job, err)) {
            return false;
        }

        const auto plan_result = build_render_plan(context.scene, job);
        if (!plan_result.value.has_value()) {
            print_diagnostics(plan_result.diagnostics, err);
            return false;
        }
        render_plan = *plan_result.value;

        const auto execution_result = build_render_execution_plan(*render_plan, context.assets.size());
        if (!execution_result.value.has_value()) {
            print_diagnostics(execution_result.diagnostics, err);
            return false;
        }
        execution_plan = *execution_result.value;
    }

    if (options.json_output) {
        out << make_inspect_report_json(context.scene, context.assets, render_plan, execution_plan) << '\n';
        return true;
    }

    print_inspect_report_text(context.scene, context.assets, render_plan, execution_plan, out);
    return true;
}

bool run_render_command(const CliOptions& options, std::ostream& out, std::ostream& err) {
    SceneContext context;
    if (!load_scene_context(options.scene_path, context, err)) {
        return false;
    }

    RenderJob job;
    if (!load_render_job(options.job_path, job, err)) {
        return false;
    }

    if (!options.output_override.empty()) {
        job.output.destination.path = options.output_override.string();
    }

    const auto plan_result = build_render_plan(context.scene, job);
    if (!plan_result.value.has_value()) {
        print_diagnostics(plan_result.diagnostics, err);
        return false;
    }

    const auto execution_result = build_render_execution_plan(*plan_result.value, context.assets.size());
    if (!execution_result.value.has_value()) {
        print_diagnostics(execution_result.diagnostics, err);
        return false;
    }

    const auto first_frame = execution_result.value->frame_tasks.empty()
        ? RasterizedFrame2D{}
        : render_frame_2d_stub(*plan_result.value, execution_result.value->frame_tasks.front());

    if (options.json_output) {
        out << make_render_report_json(context.scene, context.assets, *plan_result.value, *execution_result.value, first_frame) << '\n';
        return true;
    }

    out << "scene file: " << options.scene_path.string() << '\n';
    out << "job file: " << options.job_path.string() << '\n';
    print_execution_plan(*execution_result.value, first_frame, out);
    return true;
}

} // namespace

int run_cli(int argc, char** argv) {
    const auto parsed = parse_cli_options(argc, argv);
    if (!parsed.value.has_value()) {
        print_diagnostics(parsed.diagnostics, std::cerr);
        return 1;
    }

    const CliOptions& options = *parsed.value;
    if (options.command == "version") {
        std::cout << version_string() << '\n';
        return 0;
    }

    if (options.command == "validate") {
        if (options.scene_path.empty()) {
            std::cerr << "--scene is required\n";
            return 1;
        }

        if (!run_validate_command(options, std::cout, std::cerr)) {
            return 2;
        }
        return 0;
    }

    if (options.command == "inspect") {
        if (options.scene_path.empty()) {
            std::cerr << "--scene is required\n";
            return 1;
        }

        if (!run_inspect_command(options, std::cout, std::cerr)) {
            return 2;
        }
        return 0;
    }

    if (options.command == "render") {
        if (options.scene_path.empty() || options.job_path.empty()) {
            std::cerr << "--scene and --job are required\n";
            return 1;
        }

        if (!run_render_command(options, std::cout, std::cerr)) {
            return 2;
        }
        return 0;
    }

    print_help(std::cerr);
    return 1;
}

} // namespace tachyon
