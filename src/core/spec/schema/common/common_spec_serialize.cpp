#include "tachyon/core/spec/schema/common/common_spec.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tachyon {

void to_json(json& j, const EffectSpec& e) {
    j["type"] = e.type;
    j["enabled"] = e.enabled;
    if (!e.scalars.empty()) {
        json scalars_obj;
        for (const auto& [key, val] : e.scalars) {
            scalars_obj[key] = val;
        }
        j["scalars"] = scalars_obj;
    }
    if (!e.colors.empty()) {
        json colors_obj;
        for (const auto& [key, val] : e.colors) {
            colors_obj[key] = val;
        }
        j["colors"] = colors_obj;
    }
    if (!e.strings.empty()) {
        json strings_obj;
        for (const auto& [key, val] : e.strings) {
            strings_obj[key] = val;
        }
        j["strings"] = strings_obj;
    }
}

void from_json(const json& j, EffectSpec& e) {
    if (j.contains("type") && j.at("type").is_string()) e.type = j.at("type").get<std::string>();
    if (j.contains("enabled") && j.at("enabled").is_boolean()) e.enabled = j.at("enabled").get<bool>();
    if (j.contains("scalars") && j.at("scalars").is_object()) {
        for (auto& [key, val] : j.at("scalars").items()) {
            if (val.is_number()) e.scalars[key] = val.get<double>();
        }
    }
    if (j.contains("colors") && j.at("colors").is_object()) {
        for (auto& [key, val] : j.at("colors").items()) {
            if (val.is_object()) {
                ColorSpec c;
                from_json(val, c);
                e.colors[key] = c;
            }
        }
    }
    if (j.contains("strings") && j.at("strings").is_object()) {
        for (auto& [key, val] : j.at("strings").items()) {
            if (val.is_string()) e.strings[key] = val.get<std::string>();
        }
    }
}

} // namespace tachyon
