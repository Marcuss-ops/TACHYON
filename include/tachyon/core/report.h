#pragma once

#include "tachyon/core/core.h"
#include "tachyon/media/asset_resolution.h"
#include "tachyon/renderer2d/rasterizer.h"
#include "tachyon/runtime/diagnostics.h"
#include "tachyon/runtime/render_graph.h"
#include "tachyon/runtime/render_plan.h"
#include "tachyon/runtime/render_job.h"
#include "tachyon/spec/scene_spec.h"

#include <optional>
#include <ostream>
#include <string>

namespace tachyon {

constexpr const char* VERSION_REPORT_SCHEMA_NAME = "tachyon.version.report";
constexpr const char* VALIDATE_REPORT_SCHEMA_NAME = "tachyon.validate.report";
constexpr const char* INSPECT_REPORT_SCHEMA_NAME = "tachyon.inspect.report";
constexpr const char* RENDER_REPORT_SCHEMA_NAME = "tachyon.render.report";

constexpr const char* VERSION_REPORT_SCHEMA_VERSION = "1.0";
constexpr const char* VALIDATE_REPORT_SCHEMA_VERSION = "1.1";
constexpr const char* INSPECT_REPORT_SCHEMA_VERSION = "1.1";
constexpr const char* RENDER_REPORT_SCHEMA_VERSION = "1.1";

constexpr const char* REPORT_STATUS_OK = "ok";
constexpr const char* REPORT_STATUS_WARNING = "warning";
constexpr const char* REPORT_STATUS_ERROR = "error";

void print_inspect_report_text(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    const std::optional<RenderPlan>& render_plan,
    const std::optional<RenderExecutionPlan>& execution_plan,
    std::ostream& out);

std::string make_version_report_json(const DiagnosticBag& diagnostics);

std::string make_inspect_report_json(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    const std::optional<RenderPlan>& render_plan,
    const std::optional<RenderExecutionPlan>& execution_plan,
    const DiagnosticBag& diagnostics);

std::string make_validate_report_json(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    bool scene_valid,
    bool job_valid,
    const std::optional<RenderJob>& job,
    const DiagnosticBag& diagnostics);

std::string make_render_report_json(
    const SceneSpec& scene,
    const AssetResolutionTable& assets,
    const RenderPlan& render_plan,
    const RenderExecutionPlan& execution_plan,
    const RasterizedFrame2D& first_frame,
    const DiagnosticBag& diagnostics);

} // namespace tachyon
