#pragma once

#include "tachyon/media/asset_resolution.h"
#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/runtime/core/render_graph.h"
#include "tachyon/runtime/core/diagnostics.h"
#include "tachyon/runtime/execution/render_plan.h"
#include "tachyon/runtime/execution/render_job.h"
#include "tachyon/core/spec/scene_spec.h"

#include <optional>
#include <ostream>
#include <string>

namespace tachyon {

constexpr const char* INSPECT_REPORT_SCHEMA_VERSION = "1.0";
constexpr const char* VALIDATE_REPORT_SCHEMA_VERSION = "1.0";
constexpr const char* RENDER_REPORT_SCHEMA_VERSION = "1.0";
constexpr const char* REPORT_STATUS_OK = "ok";

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

std::string make_validate_report_json(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    bool scene_valid,
    bool job_valid,
    const std::optional<RenderJob>& job);

std::string make_render_report_json(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    const RenderPlan& render_plan,
    const RenderExecutionPlan& execution_plan,
    const DiagnosticBag& diagnostics,
    const RasterizedFrame2D& first_frame);

} // namespace tachyon
