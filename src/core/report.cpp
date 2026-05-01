#include "tachyon/core/report.h"
#include "tachyon/core/spec/json_report_utils.h"

#include <cstddef>
#include <utility>

namespace tachyon {
namespace {

using json = nlohmann::json;
using spec::diagnostics_to_json;
using spec::frame_range_to_json;
using spec::output_destination_to_json;
using spec::output_profile_to_json;

json make_diagnostics_json(const DiagnosticBag& diagnostics) {
    return diagnostics_to_json(diagnostics);
}

json make_scene_json(const SceneSpec& scene) {
    json result;
    result["schema_version"] = scene.schema_version.to_string();
    result["project"] = {
        {"id", scene.project.id},
        {"name", scene.project.name},
        {"authoring_tool", scene.project.authoring_tool}
    };
    if (scene.project.root_seed.has_value()) {
        result["project"]["root_seed"] = *scene.project.root_seed;
    }
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
        {"quality_tier", plan.quality_tier},
        {"compositing_alpha_mode", plan.compositing_alpha_mode},
        {"working_space", plan.working_space},
        {"composition", {
            {"id", plan.composition.id},
            {"name", plan.composition.name},
            {"width", plan.composition.width},
            {"height", plan.composition.height},
            {"duration", plan.composition.duration},
            {"layer_count", plan.composition.layer_count},
            {"solid_layer_count", plan.composition.solid_layer_count},
            {"shape_layer_count", plan.composition.shape_layer_count},
            {"mask_layer_count", plan.composition.mask_layer_count},
            {"image_layer_count", plan.composition.image_layer_count},
            {"text_layer_count", plan.composition.text_layer_count},
            {"precomp_layer_count", plan.composition.precomp_layer_count},
            {"track_matte_layer_count", plan.composition.track_matte_layer_count}
        }},
        {"motion_blur_enabled", plan.motion_blur_enabled},
        {"motion_blur_samples", plan.motion_blur_samples},
        {"motion_blur_shutter_angle", plan.motion_blur_shutter_angle},
        {"seed_policy_mode", plan.seed_policy_mode},
        {"compatibility_mode", plan.compatibility_mode},
        {"frame_range", frame_range_to_json(plan.frame_range)},
        {"output", {
            {"destination", output_destination_to_json(plan.output.destination)},
            {"profile", output_profile_to_json(plan.output.profile)}
        }}
    };
}

json make_render_job_json(const RenderJob& job) {
    return {
        {"job_id", job.job_id},
        {"scene_ref", job.scene_ref},
        {"composition_target", job.composition_target},
        {"frame_range", frame_range_to_json(job.frame_range)},
        {"output", output_destination_to_json(job.output.destination)}
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
    out << "  schema version: " << scene.schema_version.to_string() << '\n';
    out << "  authoring tool: " << scene.project.authoring_tool << '\n';
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
        out << "  quality tier: " << render_plan->quality_tier << '\n';
        out << "  compositing alpha: " << render_plan->compositing_alpha_mode << '\n';
        out << "  working space: " << render_plan->working_space << '\n';
        out << "  layer counts: solid=" << render_plan->composition.solid_layer_count
            << " shape=" << render_plan->composition.shape_layer_count
            << " mask=" << render_plan->composition.mask_layer_count
            << " image=" << render_plan->composition.image_layer_count
            << " text=" << render_plan->composition.text_layer_count
            << " precomp=" << render_plan->composition.precomp_layer_count
            << " matte=" << render_plan->composition.track_matte_layer_count << '\n';
        out << "  motion blur: " << (render_plan->motion_blur_enabled ? "enabled" : "disabled") << '\n';
        if (render_plan->motion_blur_enabled) {
            out << "  motion blur samples: " << render_plan->motion_blur_samples << '\n';
            out << "  shutter angle: " << render_plan->motion_blur_shutter_angle << '\n';
        }
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
    report["report_type"] = "inspect";
    report["scene"] = make_scene_json(scene);
    report["assets"] = make_assets_json(assets);
    report["status"] = REPORT_STATUS_OK;
    report["diagnostics"] = make_diagnostics_json(DiagnosticBag{});
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
    const std::optional<RenderJob>& job,
    const core::ValidationResult& validation_result) {
    json report;
    report["report_type"] = "validate";
    report["schema_version"] = VALIDATE_REPORT_SCHEMA_VERSION;
    report["scene"] = make_scene_json(scene);
    report["scene_valid"] = scene_valid;
    report["job_valid"] = job_valid;
    report["assets"] = make_assets_json(assets);
    report["status"] = validation_result.is_valid() ? REPORT_STATUS_OK : REPORT_STATUS_ERROR;
    report["diagnostics"] = make_diagnostics_json(DiagnosticBag{});
    
    // Add validation issues
    json issues_json = json::array();
    for (const auto& issue : validation_result.issues) {
        json issue_obj;
        switch (issue.severity) {
            case core::ValidationIssue::Severity::Fatal: issue_obj["severity"] = "fatal"; break;
            case core::ValidationIssue::Severity::Error: issue_obj["severity"] = "error"; break;
            case core::ValidationIssue::Severity::Warning: issue_obj["severity"] = "warning"; break;
        }
        issue_obj["path"] = issue.path;
        issue_obj["message"] = issue.message;
        issues_json.push_back(issue_obj);
    }
    report["validation_issues"] = issues_json;
    report["validation_summary"] = {
        {"errors", validation_result.error_count},
        {"warnings", validation_result.warning_count},
        {"fatal", validation_result.fatal_count}
    };
    
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
    const DiagnosticBag& diagnostics,
    const RasterizedFrame2D& first_frame,
    std::size_t cache_hits,
    std::size_t cache_misses) {
    json report;
    report["report_type"] = "render";
    report["schema_version"] = RENDER_REPORT_SCHEMA_VERSION;
    report["scene"] = make_scene_json(scene);
    report["assets"] = make_assets_json(assets);
    report["status"] = diagnostics.ok() && !diagnostics.has_warnings() ? REPORT_STATUS_OK : "warning";
    report["diagnostics"] = make_diagnostics_json(diagnostics);
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
    
    // Add cache statistics
    std::size_t total_ops = cache_hits + cache_misses;
    double hit_rate = (total_ops > 0) ? static_cast<double>(cache_hits) / static_cast<double>(total_ops) : 0.0;
    report["cache"] = {
        {"hits", cache_hits},
        {"misses", cache_misses},
        {"hit_rate", hit_rate}
    };
    
    return report.dump(2);
}

} // namespace tachyon
