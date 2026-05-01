#include "tachyon/core/spec/schema/objects/scene_spec_core.h"

using json = nlohmann::json;

namespace tachyon {

namespace {

TransitionKind transition_kind_from_string(const std::string& type) {
    if (type == "fade" || type == "fade_to_black") return TransitionKind::Fade;
    if (type == "zoom" || type == "zoom_in" || type == "zoom_out" || type == "zoom_blur") return TransitionKind::Zoom;
    if (type == "slide_left" || type == "slide_right" || type == "slide_up" || type == "slide_down" || type == "push_left" || type == "slide_easing") return TransitionKind::Slide;
    if (type == "flip" || type == "spin") return TransitionKind::Flip;
    if (type == "wipe_linear" || type == "wipe_angular" || type == "directional_blur_wipe") return TransitionKind::Wipe;
    if (type == "luma_dissolve" || type == "pixelate" || type == "glitch_slice" || type == "rgb_split") return TransitionKind::Dissolve;
    return TransitionKind::Custom;
}

void parse_spring_params(const json& object, LayerTransitionSpec::SpringSpec& spring) {
    if (!object.is_object()) return;
    if (object.contains("stiffness") && object.at("stiffness").is_number()) spring.stiffness = object.at("stiffness").get<double>();
    if (object.contains("damping") && object.at("damping").is_number()) spring.damping = object.at("damping").get<double>();
    if (object.contains("mass") && object.at("mass").is_number()) spring.mass = object.at("mass").get<double>();
    if (object.contains("velocity") && object.at("velocity").is_number()) spring.velocity = object.at("velocity").get<double>();
}

} // namespace

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
    // Convert string type to TransitionKind for internal use
    if (!out.type.empty()) {
        out.kind = transition_kind_from_string(out.type);
        out.transition_id = out.type; // Backward compatible: populate transition_id
    }
    read_string(object, "direction", out.direction);
    read_number(object, "duration", out.duration);
    read_number(object, "delay", out.delay);
    
    if (object.contains("easing")) {
        out.easing = parse_easing_preset(object.at("easing"));
        if (out.easing == animation::EasingPreset::Spring) {
            if (object.contains("spring")) {
                parse_spring_params(object.at("spring"), out.spring);
            }
        }
    }
}

} // namespace tachyon
