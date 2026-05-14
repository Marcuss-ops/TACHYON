#include "tachyon/presets/effects/effect_specs.h"
#include "simdjson.h"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace tachyon::presets {

EffectPresetSpec load_json_preset(const std::filesystem::path& path) {
    simdjson::dom::parser parser;
    simdjson::dom::element json;
    auto error = parser.load(path.string()).get(json);
    if (error) {
        throw std::runtime_error("Failed to load preset JSON: " + path.string() + " (" + simdjson::error_message(error) + ")");
    }
    
    EffectPresetSpec spec;
    spec.id = std::string(json["id"].get_string().value());
    spec.metadata.id = spec.id;
    spec.metadata.display_name = std::string(json["name"].get_string().value());
    spec.metadata.category = std::string(json["category"].get_string().value());
    
    std::string kind_id = std::string(json["kind"].get_string().value());
    
    // Parse Parameters for Schema
    std::vector<registry::ParameterDef> param_defs;
    auto params_json = json["parameters"].get_array();
    for (auto p : params_json) {
        registry::ParameterDef pd;
        pd.id = std::string(p["name"].get_string().value());
        pd.label = std::string(p["label"].get_string().value());
        
        std::string_view desc;
        if (p["description"].get(desc) == simdjson::SUCCESS) pd.description = std::string(desc);
        
        std::string type_str = std::string(p["type"].get_string().value());
        if (type_str == "float") {
            pd.type = registry::ParameterType::Float;
            pd.default_value = p["default"].get_double().value();
            
            double min_val, max_val;
            if (p["min"].get(min_val) == simdjson::SUCCESS) pd.min_value = min_val;
            if (p["max"].get(max_val) == simdjson::SUCCESS) pd.max_value = max_val;
        } else if (type_str == "string") {
            pd.type = registry::ParameterType::String;
            pd.default_value = std::string(p["default"].get_string().value());
        }
        param_defs.push_back(pd);
    }
    spec.schema.params = param_defs;
    
    // Mappings
    std::map<std::string, std::string> mapping_map;
    auto mappings = json["mappings"].get_object();
    for (auto field : mappings) {
        mapping_map[std::string(field.key)] = std::string(field.value.get_string().value());
    }
    
    spec.factory = [kind_id, mapping_map](const registry::ParameterBag& params) {
        EffectSpec effect;
        effect.type = kind_id;
        for (const auto& [param_name, target_key] : mapping_map) {
            if (params.contains(param_name)) {
                 // Try double first
                 auto val_double = params.get<double>(param_name);
                 if (val_double) {
                     effect.set_param(target_key, static_cast<float>(*val_double));
                 } else {
                     auto val_string = params.get<std::string>(param_name);
                     if (val_string) effect.set_param(target_key, *val_string);
                 }
            }
        }
        return effect;
    };
    
    return spec;
}

void load_presets_from_directory(const std::filesystem::path& dir, std::vector<EffectPresetSpec>& out_specs) {
    if (!std::filesystem::exists(dir)) return;
    
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.path().extension() == ".json") {
            try {
                out_specs.push_back(load_json_preset(entry.path()));
            } catch (const std::exception& e) {
                std::cerr << "[Presets] Failed to load preset '" << entry.path().filename() << "': " << e.what() << std::endl;
            }
        }
    }
}

} // namespace tachyon::presets
