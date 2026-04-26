#include "tachyon/renderer2d/spec/gradient_spec.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tachyon {

// JSON serialization for GradientStop
void to_json(json& j, const GradientStop& s) {
    j = json{{"offset", s.offset}, {"color", s.color}};
}

void from_json(const json& j, GradientStop& s) {
    if (j.contains("offset") && j.at("offset").is_number()) s.offset = j.at("offset").get<float>();
    if (j.contains("color") && j.at("color").is_object()) from_json(j.at("color"), s.color);
}

// JSON serialization for AnimatedGradientStop
void to_json(json& j, const AnimatedGradientStop& s) {
    j = json{{"offset", s.offset}, {"color", s.color}};
}

void from_json(const json& j, AnimatedGradientStop& s) {
    if (j.contains("offset")) s.offset = j.at("offset").get<AnimatedScalarSpec>();
    if (j.contains("color")) s.color = j.at("color").get<AnimatedColorSpec>();
}

// JSON serialization for GradientSpec
void to_json(json& j, const GradientSpec& g) {
    j["type"] = (g.type == GradientType::Radial) ? "radial" : "linear";
    j["start"] = json{{"x", g.start.x}, {"y", g.start.y}};
    j["end"] = json{{"x", g.end.x}, {"y", g.end.y}};
    j["radial_radius"] = g.radial_radius;
    j["stops"] = g.stops;
}

void from_json(const json& j, GradientSpec& g) {
    if (j.contains("type") && j.at("type").is_string()) {
        std::string t = j.at("type").get<std::string>();
        g.type = (t == "radial") ? GradientType::Radial : GradientType::Linear;
    }
    if (j.contains("start") && j.at("start").is_object()) {
        auto& s = j.at("start");
        if (s.contains("x") && s.at("x").is_number()) g.start.x = s.at("x").get<float>();
        if (s.contains("y") && s.at("y").is_number()) g.start.y = s.at("y").get<float>();
    }
    if (j.contains("end") && j.at("end").is_object()) {
        auto& e = j.at("end");
        if (e.contains("x") && e.at("x").is_number()) g.end.x = e.at("x").get<float>();
        if (e.contains("y") && e.at("y").is_number()) g.end.y = e.at("y").get<float>();
    }
    if (j.contains("radial_radius") && j.at("radial_radius").is_number()) g.radial_radius = j.at("radial_radius").get<float>();
    if (j.contains("stops") && j.at("stops").is_array()) g.stops = j.at("stops").get<std::vector<GradientStop>>();
}

// JSON serialization for AnimatedGradientSpec
void to_json(json& j, const AnimatedGradientSpec& g) {
    j["type"] = g.type;
    if (!g.angle.empty()) j["angle"] = g.angle;
    if (!g.center.empty()) j["center"] = g.center;
    if (!g.radius.empty()) j["radius"] = g.radius;
    if (!g.stops.empty()) j["stops"] = g.stops;
}

void from_json(const json& j, AnimatedGradientSpec& g) {
    if (j.contains("type") && j.at("type").is_string()) g.type = j.at("type").get<std::string>();
    if (j.contains("angle")) g.angle = j.at("angle").get<AnimatedScalarSpec>();
    if (j.contains("center")) g.center = j.at("center").get<AnimatedVector2Spec>();
    if (j.contains("radius")) g.radius = j.at("radius").get<AnimatedScalarSpec>();
    if (j.contains("stops") && j.at("stops").is_array()) g.stops = j.at("stops").get<std::vector<AnimatedGradientStop>>();
}

} // namespace tachyon
