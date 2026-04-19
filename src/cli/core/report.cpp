#include "tachyon/core/report.h"

#include "nlohmann/json.hpp"

#include <cstddef>
#include <utility>

namespace tachyon {
namespace {

using json = nlohmann::json;

json make_scene_json(const SceneSpec& scene) {
    json result;
    result["spec_version"] = scene.spec_version;
    result["project"] = {
        {"id", scene.project.id},
        {"name", scene.project.name}
    };
    result["asset_count"] = scene.assets.size();
    result["composition_count"] = scene.compositions.size();
    return result;
}

json make_assets_json(const AssetResolutionTable& assets) {
    json result = json::array();
    for (const auto& [asset_id, asset] : assets) {
        result.push_back({
            {"asset_id", asset_id},
            {"type", asset.type},
            {"absolute_path", asset.absolute_path.string()},
            {"exists", asset.exists}
        });
    }
    return result;
}

json make_render_plan_json(const RenderPlan& plan) {
    return {
        {"job_id", plan.job_id},
        {"scene_ref", plan.scene_ref},
        {"composition_target", plan.composition_target},
        {"frame_range", {
            {"start", plan.frame_range.start},
            {"end", plan.frame_range.end}
        }},
        {"output", {
            {"destination", {
                {"path", plan.output.destination.path},
                {"overwrite", plan.output.destination.overwrite}
            }},
            {"profile", {
                {"name", plan.output.profile.name},
                {"class", plan.output.profile.class_name},
                {"container", plan.output.profile.container}
            }}
        }}
    };
}

json make_render_job_json(const RenderJob& job) {
    return {
        {"job_id", job.job_id},
        {"scene_ref", job.scene_ref},
        {"composition_target", job.composition_target},
        {"frame_range", {
            {"start", job.frame_range.start},
            {"end", job.frame_range.end}
        }},
        {"output", {
            {"path", job.output.destination.path},
            {"overwrite", job.output.destination.overwrite}
        }}
    };
}

json make_render_graph_json(const RenderExecutionPlan& execution_plan) {
    json steps = json::array();
    for (const auto& step : execution_plan.steps) {
        json item = {
            {"id", step.id},
            {"kind", render_step_kind_string(step.kind)},
            {"label", step.label},
            {"dependencies", step.dependencies}
        };
        if (step.frame_number.has_value()) {
            item["frame_number"] = *step.frame_number;
        }
        steps.push_back(std::move(item));
    }

    json frame_tasks = json::array();
    for (const auto& task : execution_plan.frame_tasks) {
        frame_tasks.push_back({
            {"frame_number", task.frame_number},
            {"cache_key", task.cache_key.value},
            {"cacheable", task.cacheable},
            {"invalidates_when_changed", task.invalidates_when_changed}
        });
    }

    return {
        {"resolved_asset_count", execution_plan.resolved_asset_count},
        {"steps", std::move(steps)},
        {"frame_tasks", std::move(frame_tasks)}
    };
}

} // namespace

void print_inspect_report_text(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    const std::optional<RenderPlan>& render_plan,
    const std::optional<RenderExecutionPlan>& execution_plan,
    std::ostream& out) {
    out << "scene summary\n";
    out << "  project: " << scene.project.id << " / " << scene.project.name << '\n';
    out << "  spec version: " << scene.spec_version << '\n';
    out << "  assets: " << scene.assets.size() << '\n';
    out << "  compositions: " << scene.compositions.size() << '\n';

    out << "assets\n";
    out << "  resolved: " << assets.size() << '\n';
    for (const auto& [asset_id, asset] : assets) {
        out << "  - " << asset_id << " [" << asset.type << "] -> " << asset.absolute_path.string() << '\n';
    }

    if (render_plan.has_value()) {
        out << "render plan\n";
        out << "  job: " << render_plan->job_id << '\n';
        out << "  scene ref: " << render_plan->scene_ref << '\n';
        out << "  composition target: " << render_plan->composition_target << '\n';
        out << "  frame range: " << render_plan->frame_range.start << " -> " << render_plan->frame_range.end << '\n';
        out << "  output: " << render_plan->output.destination.path << '\n';
    }

    if (execution_plan.has_value()) {
        out << "render graph\n";
        out << "  resolved assets: " << execution_plan->resolved_asset_count << '\n';
        out << "  steps: " << execution_plan->steps.size() << '\n';
        for (const auto& step : execution_plan->steps) {
            out << "  - " << step.id << " [" << render_step_kind_string(step.kind) << "]";
            if (step.frame_number.has_value()) {
                out << " frame=" << *step.frame_number;
            }
            if (!step.dependencies.empty()) {
                out << " deps=";
                for (std::size_t index = 0; index < step.dependencies.size(); ++index) {
                    if (index > 0) {
                        out << ',';
                    }
                    out << step.dependencies[index];
                }
            }
            out << '\n';
        }
        out << "  frame tasks: " << execution_plan->frame_tasks.size() << '\n';
        for (const auto& task : execution_plan->frame_tasks) {
            out << "  - frame " << task.frame_number << " cache=" << task.cache_key.value << '\n';
            out << "    invalidates_when_changed: ";
            for (std::size_t index = 0; index < task.invalidates_when_changed.size(); ++index) {
                if (index > 0) {
                    out << ", ";
                }
                out << task.invalidates_when_changed[index];
            }
            out << '\n';
        }
    }
}

std::string make_inspect_report_json(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    const std::optional<RenderPlan>& render_plan,
    const std::optional<RenderExecutionPlan>& execution_plan) {
    json report;
    report["scene"] = make_scene_json(scene);
    report["assets"] = make_assets_json(assets);
    if (render_plan.has_value()) {
        report["render_plan"] = make_render_plan_json(*render_plan);
    }
    if (execution_plan.has_value()) {
        report["render_graph"] = make_render_graph_json(*execution_plan);
    }
    report["schema_version"] = INSPECT_REPORT_SCHEMA_VERSION;
    return report.dump(2);
}

std::string make_validate_report_json(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    bool scene_valid,
    bool job_valid,
    const std::optional<RenderJob>& job) {
    json report;
    report["schema_version"] = VALIDATE_REPORT_SCHEMA_VERSION;
    report["scene"] = make_scene_json(scene);
    report["scene_valid"] = scene_valid;
    report["job_valid"] = job_valid;
    report["assets"] = make_assets_json(assets);
    if (job.has_value()) {
        report["job"] = make_render_job_json(*job);
    }
    return report.dump(2);
}

std::string make_render_report_json(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    const RenderPlan& render_plan,
    const RenderExecutionPlan& execution_plan,
    const RasterizedFrame2D& first_frame) {
    json report;
    report["schema_version"] = RENDER_REPORT_SCHEMA_VERSION;
    report["scene"] = make_scene_json(scene);
    report["assets"] = make_assets_json(assets);
    report["render_plan"] = make_render_plan_json(render_plan);
    report["render_graph"] = make_render_graph_json(execution_plan);
    report["first_frame"] = {
        {"frame_number", first_frame.frame_number},
        {"width", first_frame.width},
        {"height", first_frame.height},
        {"layer_count", first_frame.layer_count},
        {"estimated_draw_ops", first_frame.estimated_draw_ops},
        {"backend_name", first_frame.backend_name},
        {"cache_key", first_frame.cache_key},
        {"note", first_frame.note}
    };
    return report.dump(2);
}

} // namespace tachyon
