#include "tachyon/core/spec/schema/objects/scene_spec_core.h"

using json = nlohmann::json;

namespace tachyon {

void parse_track_bindings(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (!object.contains("track_bindings") || !object.at("track_bindings").is_array()) return;
    const auto& bindings = object.at("track_bindings");
    for (std::size_t i = 0; i < bindings.size(); ++i) {
        if (!bindings[i].is_object()) continue;
        const auto& b = bindings[i];
        spec::TrackBinding binding;
        read_string(b, "property_path", binding.property_path);
        read_string(b, "source_id", binding.source_id);
        read_string(b, "source_track_name", binding.source_track_name);
        read_number(b, "influence", binding.influence);
        read_bool(b, "enabled", binding.enabled);
        layer.track_bindings.push_back(std::move(binding));
    }
}

void parse_time_remap(const json& object, LayerSpec& layer, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (!object.contains("time_remap") || !object.at("time_remap").is_object()) return;
    const auto& tr = object.at("time_remap");
    read_bool(tr, "enabled", layer.time_remap.enabled);
    
    if (tr.contains("mode") && tr.at("mode").is_string()) {
        const std::string mode = tr.at("mode").get<std::string>();
        if (mode == "hold") layer.time_remap.mode = spec::TimeRemapMode::Hold;
        else if (mode == "blend") layer.time_remap.mode = spec::TimeRemapMode::Blend;
        else if (mode == "optical_flow") layer.time_remap.mode = spec::TimeRemapMode::OpticalFlow;
    }
    
    if (tr.contains("keyframes") && tr.at("keyframes").is_array()) {
        const auto& kfs = tr.at("keyframes");
        for (const auto& kf : kfs) {
            if (kf.is_array() && kf.size() >= 2) {
                layer.time_remap.keyframes.push_back({kf[0].get<float>(), kf[1].get<float>()});
            } else if (kf.is_object()) {
                float src = 0.0f, dest = 0.0f;
                read_number(kf, "source_time", src);
                read_number(kf, "dest_time", dest);
                layer.time_remap.keyframes.push_back({src, dest});
            }
        }
    }
}

void parse_transition(const json& object, LayerTransitionSpec& out, const std::string& path, DiagnosticBag& diagnostics) {
    (void)path; (void)diagnostics;
    if (!object.is_object()) return;
    read_string(object, "type", out.type);
    read_string(object, "direction", out.direction);
    read_number(object, "duration", out.duration);
    read_number(object, "delay", out.delay);
    
    if (object.contains("easing")) {
        out.easing = parse_easing_preset(object.at("easing"));
        if (out.easing == animation::EasingPreset::Spring) {
            parse_spring_params(object.at("spring"), out.spring);
        }
    }
}

} // namespace tachyon
