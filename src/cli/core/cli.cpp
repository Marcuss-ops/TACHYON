#include "tachyon/core/cli.h"

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
#include <string>
#include <vector>

namespace tachyon {
namespace {

void print_diagnostics(const DiagnosticBag& diagnostics, std::ostream& out) {
    for (const auto& diagnostic : diagnostics.diagnostics) {
        out << diagnostic.code << ": " << diagnostic.message;
        if (!diagnostic.path.empty()) {
            out << " (" << diagnostic.path << ")";
        }
        out << '\n';
    }
}

std::string require_argument(const std::vector<std::string>& args, std::size_t& index) {
    if (index + 1 >= args.size()) {
        return {};
    }
    ++index;
    return args[index];
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
    out << "  tachyon validate --scene <file> [--job <file>]\n";
    out << "  tachyon inspect --scene <file> [--job <file>] [--json]\n";
    out << "  tachyon render --scene <file> --job <file> [--out <file>]\n";
}

bool validate_scene_file(const std::filesystem::path& scene_path, std::ostream& out, std::ostream& err, bool print_success) {
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

    if (print_success) {
        out << "scene spec valid\n";
    }
    return true;
}

bool validate_job_file(const std::filesystem::path& job_path, std::ostream& out, std::ostream& err, bool print_success) {
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

    if (print_success) {
        out << "render job valid\n";
    }
    return true;
}

void print_execution_plan(const RenderExecutionPlan& execution_plan, const RasterizedFrame2D& rasterized_first_frame, std::ostream& out) {
    out << "render execution plan valid\n";
    out << "scene: " << execution_plan.render_plan.scene_ref << '\n';
    out << "job: " << execution_plan.render_plan.job_id << '\n';
    out << "composition: " << execution_plan.render_plan.composition_target << " (" << execution_plan.render_plan.composition.width << "x" << execution_plan.render_plan.composition.height
        << " @ " << execution_plan.render_plan.composition.frame_rate.value() << " fps, " << execution_plan.render_plan.composition.layer_count << " layers)\n";
    out << "frames: " << execution_plan.render_plan.frame_range.start << " -> " << execution_plan.render_plan.frame_range.end << '\n';
    out << "resolved assets: " << execution_plan.resolved_asset_count << '\n';
    out << "graph steps: " << execution_plan.steps.size() << '\n';
    out << "frame tasks: " << execution_plan.frame_tasks.size() << '\n';
    out << "first frame cache key: " << rasterized_first_frame.cache_key << '\n';
    out << "2d stub backend: " << rasterized_first_frame.backend_name << '\n';
    out << "2d stub draw ops: " << rasterized_first_frame.estimated_draw_ops << '\n';
    out << "note: pixel rendering is not wired yet; this command validates the graph and stub raster slice\n";
}

bool inspect_scene_file(const std::filesystem::path& scene_path, const std::filesystem::path& job_path, bool json_output, std::ostream& out, std::ostream& err) {
    const auto scene = parse_scene_spec_file(scene_path);
    if (!scene.value.has_value()) {
        print_diagnostics(scene.diagnostics, err);
        return false;
    }

    const auto scene_validation = validate_scene_spec(*scene.value);
    if (!scene_validation.ok()) {
        print_diagnostics(scene_validation.diagnostics, err);
        return false;
    }

    const auto assets = resolve_assets(*scene.value, scene_asset_root(scene_path));
    if (!assets.value.has_value()) {
        print_diagnostics(assets.diagnostics, err);
        return false;
    }

    std::optional<RenderPlan> render_plan;
    std::optional<RenderExecutionPlan> execution_plan;
    if (!job_path.empty()) {
        const auto job = parse_render_job_file(job_path);
        if (!job.value.has_value()) {
            print_diagnostics(job.diagnostics, err);
            return false;
        }

        const auto job_validation = validate_render_job(*job.value);
        if (!job_validation.ok()) {
            print_diagnostics(job_validation.diagnostics, err);
            return false;
        }

        const auto plan_result = build_render_plan(*scene.value, *job.value);
        if (!plan_result.value.has_value()) {
            print_diagnostics(plan_result.diagnostics, err);
            return false;
        }
        render_plan = *plan_result.value;

        const auto execution_result = build_render_execution_plan(*render_plan, assets.value->size());
        if (!execution_result.value.has_value()) {
            print_diagnostics(execution_result.diagnostics, err);
            return false;
        }
        execution_plan = *execution_result.value;
    }

    if (json_output) {
        out << make_inspect_report_json(*scene.value, *assets.value, render_plan, execution_plan) << '\n';
    } else {
        print_inspect_report_text(*scene.value, *assets.value, render_plan, execution_plan, out);
    }

    return true;
}

} // namespace

int run_cli(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) {
        print_help(std::cout);
        return 1;
    }

    const std::string command = args[0];
    if (command == "version") {
        std::cout << version_string() << '\n';
        return 0;
    }

    std::filesystem::path scene_path;
    std::filesystem::path job_path;
    std::filesystem::path output_override;
    bool json_output{false};

    for (std::size_t index = 1; index < args.size(); ++index) {
        const std::string& arg = args[index];
        if (arg == "--scene") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                std::cerr << "missing value for --scene\n";
                return 1;
            }
            scene_path = value;
            continue;
        }
        if (arg == "--job") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                std::cerr << "missing value for --job\n";
                return 1;
            }
            job_path = value;
            continue;
        }
        if (arg == "--out") {
            const std::string value = require_argument(args, index);
            if (value.empty()) {
                std::cerr << "missing value for --out\n";
                return 1;
            }
            output_override = value;
            continue;
        }
        if (arg == "--json") {
            json_output = true;
            continue;
        }
    }

    if (command == "validate") {
        if (scene_path.empty()) {
            std::cerr << "--scene is required\n";
            return 1;
        }

        if (!validate_scene_file(scene_path, std::cout, std::cerr, true)) {
            return 2;
        }

        if (!job_path.empty() && !validate_job_file(job_path, std::cout, std::cerr, true)) {
            return 2;
        }

        return 0;
    }

    if (command == "inspect") {
        if (scene_path.empty()) {
            std::cerr << "--scene is required\n";
            return 1;
        }

        if (!inspect_scene_file(scene_path, job_path, json_output, std::cout, std::cerr)) {
            return 2;
        }

        return 0;
    }

    if (command == "render") {
        if (scene_path.empty() || job_path.empty()) {
            std::cerr << "--scene and --job are required\n";
            return 1;
        }

        if (!validate_scene_file(scene_path, std::cout, std::cerr, false)) {
            return 2;
        }

        const auto job = parse_render_job_file(job_path);
        if (!job.value.has_value()) {
            print_diagnostics(job.diagnostics, std::cerr);
            return 2;
        }

        RenderJob resolved_job = *job.value;
        if (!output_override.empty()) {
            resolved_job.output.destination.path = output_override.string();
        }

        const auto job_validation = validate_render_job(resolved_job);
        if (!job_validation.ok()) {
            print_diagnostics(job_validation.diagnostics, std::cerr);
            return 2;
        }

        const auto scene = parse_scene_spec_file(scene_path);
        if (!scene.value.has_value()) {
            print_diagnostics(scene.diagnostics, std::cerr);
            return 2;
        }

        const auto plan = build_render_plan(*scene.value, resolved_job);
        if (!plan.value.has_value()) {
            print_diagnostics(plan.diagnostics, std::cerr);
            return 2;
        }

        const auto assets = resolve_assets(*scene.value, scene_asset_root(scene_path));
        if (!assets.value.has_value()) {
            print_diagnostics(assets.diagnostics, std::cerr);
            return 2;
        }

        const auto execution_plan = build_render_execution_plan(*plan.value, assets.value->size());
        if (!execution_plan.value.has_value()) {
            print_diagnostics(execution_plan.diagnostics, std::cerr);
            return 2;
        }

        const auto first_frame = execution_plan.value->frame_tasks.empty()
            ? RasterizedFrame2D{}
            : render_frame_2d_stub(*plan.value, execution_plan.value->frame_tasks.front());

        std::cout << "scene file: " << scene_path.string() << '\n';
        std::cout << "job file: " << job_path.string() << '\n';
        print_execution_plan(*execution_plan.value, first_frame, std::cout);
        return 0;
    }

    print_help(std::cerr);
    return 1;
}

} // namespace tachyon
