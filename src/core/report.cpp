#include "tachyon/core/report.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/runtime/execution/session/render_session.h"
#include "tachyon/runtime/core/data/property_graph.h"

#include <cstddef>
#include <utility>
#include <ostream>
#include <string>
#include <vector>

namespace tachyon {



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

} // namespace tachyon
