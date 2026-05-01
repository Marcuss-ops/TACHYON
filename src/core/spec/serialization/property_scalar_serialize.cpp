#include "tachyon/core/spec/property_spec_serialize_helpers.h"

using json = nlohmann::json;

namespace tachyon {

// ScalarKeyframeSpec serialization
void to_json(json& j, const ScalarKeyframeSpec& k) {
    j["time"] = k.time;
    j["value"] = k.value;
    j["easing"] = static_cast<int>(k.easing);
    j["bezier"] = json{{"cx1", k.bezier.cx1}, {"cy1", k.bezier.cy1}, {"cx2", k.bezier.cx2}, {"cy2", k.bezier.cy2}};
    j["speed_in"] = k.speed_in;
    j["influence_in"] = k.influence_in;
    j["speed_out"] = k.speed_out;
    j["influence_out"] = k.influence_out;
}

void from_json(const json& j, ScalarKeyframeSpec& k) {
    if (j.contains("time") && j.at("time").is_number()) k.time = j.at("time").get<double>();
    if (j.contains("value") && j.at("value").is_number()) k.value = j.at("value").get<double>();
    if (j.contains("easing") && j.at("easing").is_number()) k.easing = static_cast<animation::EasingPreset>(j.at("easing").get<int>());
    if (j.contains("bezier") && j.at("bezier").is_object()) {
        auto& b = j.at("bezier");
        if (b.contains("cx1") && b.at("cx1").is_number()) k.bezier.cx1 = b.at("cx1").get<double>();
        if (b.contains("cy1") && b.at("cy1").is_number()) k.bezier.cy1 = b.at("cy1").get<double>();
        if (b.contains("cx2") && b.at("cx2").is_number()) k.bezier.cx2 = b.at("cx2").get<double>();
        if (b.contains("cy2") && b.at("cy2").is_number()) k.bezier.cy2 = b.at("cy2").get<double>();
    }
    if (j.contains("speed_in") && j.at("speed_in").is_number()) k.speed_in = j.at("speed_in").get<double>();
    if (j.contains("influence_in") && j.at("influence_in").is_number()) k.influence_in = j.at("influence_in").get<double>();
    if (j.contains("speed_out") && j.at("speed_out").is_number()) k.speed_out = j.at("speed_out").get<double>();
    if (j.contains("influence_out") && j.at("influence_out").is_number()) k.influence_out = j.at("influence_out").get<double>();
}

// AnimatedScalarSpec serialization
void to_json(json& j, const AnimatedScalarSpec& a) {
    if (a.value.has_value()) j["value"] = a.value.value();
    if (!a.keyframes.empty()) j["keyframes"] = a.keyframes;
    if (a.audio_band.has_value()) j["audio_band"] = static_cast<int>(a.audio_band.value());
    if (a.audio_min != 0.0) j["audio_min"] = a.audio_min;
    if (a.audio_max != 1.0) j["audio_max"] = a.audio_max;
    if (a.expression.has_value()) j["expression"] = a.expression.value();
    if (a.wiggle.enabled) j["wiggle"] = a.wiggle;
}

void from_json(const json& j, AnimatedScalarSpec& a) {
    if (j.contains("value") && j.at("value").is_number()) a.value = j.at("value").get<double>();
    if (j.contains("keyframes") && j.at("keyframes").is_array()) {
        a.keyframes = j.at("keyframes").get<std::vector<ScalarKeyframeSpec>>();
    }
    if (j.contains("audio_band") && j.at("audio_band").is_number()) a.audio_band = static_cast<AudioBandType>(j.at("audio_band").get<int>());
    if (j.contains("audio_min") && j.at("audio_min").is_number()) a.audio_min = j.at("audio_min").get<double>();
    if (j.contains("audio_max") && j.at("audio_max").is_number()) a.audio_max = j.at("audio_max").get<double>();
    if (j.contains("expression") && j.at("expression").is_string()) a.expression = j.at("expression").get<std::string>();
    if (j.contains("wiggle") && j.at("wiggle").is_object()) a.wiggle = j.at("wiggle").get<WiggleSpec>();
}

} // namespace tachyon
