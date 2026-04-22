#pragma once
#include "tachyon/runtime/execution/batch_runner.h"
#include "tachyon/runtime/execution/render_job.h"
#include "tachyon/core/spec/scene_spec.h"
#include "tachyon/core/spec/scene_spec_core.h"
#include "../../../core/cli/cli_internal.h"
#include "tachyon/media/asset_resolution.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>

namespace tachyon {

using json = nlohmann::json;

// Helpers
std::filesystem::path resolve_relative_path(const std::filesystem::path& base, const std::filesystem::path& value);
std::string summarize_diagnostics(const DiagnosticBag& diagnostics, const std::string& fallback);

RenderBatchSpec parse_batch_spec(const json& root, const std::filesystem::path& base_path, DiagnosticBag& diagnostics);

bool load_scene_context(
    const std::filesystem::path& scene_path,
    SceneSpec& scene,
    AssetResolutionTable& assets,
    DiagnosticBag& diagnostics);

bool load_render_job(
    const std::filesystem::path& job_path,
    RenderJob& job,
    DiagnosticBag& diagnostics);

} // namespace tachyon
