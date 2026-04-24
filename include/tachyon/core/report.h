#pragma once

#include "tachyon/media/resolution/asset_resolution.h"
#include "tachyon/renderer2d/raster/rasterizer.h"
#include "tachyon/runtime/core/graph/runtime_render_graph.h"
#include "tachyon/runtime/core/diagnostics/diagnostics.h"
#include "tachyon/runtime/execution/planning/render_plan.h"
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

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
    const std::optional<RenderJob>& job,
    const core::ValidationResult& validation_result);

std::string make_render_report_json(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    const RenderPlan& render_plan,
    const RenderExecutionPlan& execution_plan,
    const DiagnosticBag& diagnostics,
    const RasterizedFrame2D& first_frame,
    std::size_t cache_hits = 0,
    std::size_t cache_misses = 0);

} // namespace tachyon
