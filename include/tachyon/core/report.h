#pragma once

#include "tachyon/media/asset_resolution.h"
#include "tachyon/runtime/render_graph.h"
#include "tachyon/runtime/render_plan.h"
#include "tachyon/spec/scene_spec.h"

#include <optional>
#include <ostream>
#include <string>

namespace tachyon {

void print_inspect_report_text(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    const std::optional<RenderPlan>& render_plan,
    const std::optional<RenderExecutionPlan>& execution_plan,
    std::ostream& out);

std::string make_inspect_report_json(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    const std::optional<RenderPlan>& render_plan,
    const std::optional<RenderExecutionPlan>& execution_plan);

} // namespace tachyon
