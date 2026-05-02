#pragma once
#include "tachyon/runtime/execution/jobs/render_job.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <string>

namespace tachyon {

using json = nlohmann::json;

// Helpers
void flatten_variables(
    const json& value,
    const std::string& prefix,
    std::unordered_map<std::string, double>& numeric_variables,
    std::unordered_map<std::string, std::string>& string_variables);

bool is_quality_tier_valid(const std::string& tier);
bool is_alpha_mode_valid(const std::string& mode);
bool is_motion_blur_curve_valid(const std::string& curve);

OutputProfile parse_output_profile(const json& object, const std::string& path, DiagnosticBag& diagnostics);

} // namespace tachyon
