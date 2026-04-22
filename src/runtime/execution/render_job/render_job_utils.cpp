#include "render_job_internal.h"

namespace tachyon {

void flatten_variables(
    const json& value,
    const std::string& prefix,
    std::unordered_map<std::string, double>& numeric_variables,
    std::unordered_map<std::string, std::string>& string_variables) {
    if (value.is_object()) {
        for (const auto& [key, child] : value.items()) {
            const std::string next_prefix = prefix.empty() ? key : prefix + "." + key;
            flatten_variables(child, next_prefix, numeric_variables, string_variables);
        }
        return;
    }

    if (value.is_number()) {
        numeric_variables[prefix] = value.get<double>();
        return;
    }

    if (value.is_string()) {
        string_variables[prefix] = value.get<std::string>();
        return;
    }
}

bool read_double(const json& object, const char* key, double& out) {
    if (!object.contains(key) || object.at(key).is_null()) return false;
    const auto& value = object.at(key);
    if (!value.is_number()) return false;
    out = value.get<double>();
    return true;
}

bool is_quality_tier_valid(const std::string& tier) {
    return tier == "draft" || tier == "high" || tier == "cinematic";
}

bool is_alpha_mode_valid(const std::string& mode) {
    return mode == "premultiplied" || mode == "straight" || mode == "opaque";
}

bool is_motion_blur_curve_valid(const std::string& curve) {
    return curve == "box" || curve == "triangle" || curve == "gaussian";
}

} // namespace tachyon
