#include "tachyon/core/spec/property_spec_serialize_helpers.h"
#include "tachyon/renderer2d/path/mask_path.h"

using json = nlohmann::json;

namespace tachyon {

// Vector2KeyframeSpec serialization
void to_json(json& j, const Vector2KeyframeSpec& k) {
    j["time"] = k.time;
    j["value"] = json{{"x", k.value.x}, {"y", k.value.y}};
    j["tangent_in"] = json{{"x", k.tangent_in.x}, {"y", k.tangent_in.y}};
    j["tangent_out"] = json{{"x", k.tangent_out.x}, {"y", k.tangent_out.y}};
    j["easing"] = static_cast<int>(k.easing);
    j["bezier"] = json{{"cx1", k.bezier.cx1}, {"cy1", k.bezier.cy1}, {"cx2", k.bezier.cx2}, {"cy2", k.bezier.cy2}};
    j["speed_in"] = k.speed_in;
    j["influence_in"] = k.influence_in;
    j["speed_out"] = k.speed_out;
    j["influence_out"] = k.influence_out;
}

void from_json(const json& j, Vector2KeyframeSpec& k) {
    if (j.contains("time") && j.at("time").is_number()) k.time = j.at("time").get<double>();
    if (j.contains("value") && j.at("value").is_object()) {
        auto& v = j.at("value");
        if (v.contains("x") && v.at("x").is_number()) k.value.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) k.value.y = v.at("y").get<float>();
    }
    if (j.contains("tangent_in") && j.at("tangent_in").is_object()) {
        auto& v = j.at("tangent_in");
        if (v.contains("x") && v.at("x").is_number()) k.tangent_in.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) k.tangent_in.y = v.at("y").get<float>();
    }
    if (j.contains("tangent_out") && j.at("tangent_out").is_object()) {
        auto& v = j.at("tangent_out");
        if (v.contains("x") && v.at("x").is_number()) k.tangent_out.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) k.tangent_out.y = v.at("y").get<float>();
    }
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

// AnimatedVector2Spec serialization
void to_json(json& j, const AnimatedVector2Spec& a) {
    if (a.value.has_value()) j["value"] = json{{"x", a.value.value().x}, {"y", a.value.value().y}};
    if (!a.keyframes.empty()) j["keyframes"] = a.keyframes;
    if (a.expression.has_value()) j["expression"] = a.expression.value();
}

void from_json(const json& j, AnimatedVector2Spec& a) {
    if (j.contains("value") && j.at("value").is_object()) {
        auto& v = j.at("value");
        math::Vector2 val;
        if (v.contains("x") && v.at("x").is_number()) val.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) val.y = v.at("y").get<float>();
        a.value = val;
    }
    if (j.contains("keyframes") && j.at("keyframes").is_array()) {
        a.keyframes = j.at("keyframes").get<std::vector<Vector2KeyframeSpec>>();
    }
    if (j.contains("expression") && j.at("expression").is_string()) a.expression = j.at("expression").get<std::string>();
}

// AnimatedVector3Spec::Keyframe serialization
void to_json(json& j, const AnimatedVector3Spec::Keyframe& k) {
    j["time"] = k.time;
    j["value"] = json{{"x", k.value.x}, {"y", k.value.y}, {"z", k.value.z}};
    j["tangent_in"] = json{{"x", k.tangent_in.x}, {"y", k.tangent_in.y}, {"z", k.tangent_in.z}};
    j["tangent_out"] = json{{"x", k.tangent_out.x}, {"y", k.tangent_out.y}, {"z", k.tangent_out.z}};
    j["easing"] = static_cast<int>(k.easing);
    j["bezier"] = json{{"cx1", k.bezier.cx1}, {"cy1", k.bezier.cy1}, {"cx2", k.bezier.cx2}, {"cy2", k.bezier.cy2}};
    j["speed_in"] = k.speed_in;
    j["influence_in"] = k.influence_in;
    j["speed_out"] = k.speed_out;
    j["influence_out"] = k.influence_out;
}

void from_json(const json& j, AnimatedVector3Spec::Keyframe& k) {
    if (j.contains("time") && j.at("time").is_number()) k.time = j.at("time").get<double>();
    if (j.contains("value") && j.at("value").is_object()) {
        auto& v = j.at("value");
        if (v.contains("x") && v.at("x").is_number()) k.value.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) k.value.y = v.at("y").get<float>();
        if (v.contains("z") && v.at("z").is_number()) k.value.z = v.at("z").get<float>();
    }
    if (j.contains("tangent_in") && j.at("tangent_in").is_object()) {
        auto& v = j.at("tangent_in");
        if (v.contains("x") && v.at("x").is_number()) k.tangent_in.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) k.tangent_in.y = v.at("y").get<float>();
        if (v.contains("z") && v.at("z").is_number()) k.tangent_in.z = v.at("z").get<float>();
    }
    if (j.contains("tangent_out") && j.at("tangent_out").is_object()) {
        auto& v = j.at("tangent_out");
        if (v.contains("x") && v.at("x").is_number()) k.tangent_out.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) k.tangent_out.y = v.at("y").get<float>();
        if (v.contains("z") && v.at("z").is_number()) k.tangent_out.z = v.at("z").get<float>();
    }
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

// AnimatedVector3Spec serialization
void to_json(json& j, const AnimatedVector3Spec& a) {
    if (a.value.has_value()) j["value"] = json{{"x", a.value.value().x}, {"y", a.value.value().y}, {"z", a.value.value().z}};
    if (!a.keyframes.empty()) j["keyframes"] = a.keyframes;
    if (a.expression.has_value()) j["expression"] = a.expression.value();
}

void from_json(const json& j, AnimatedVector3Spec& a) {
    if (j.contains("value") && j.at("value").is_object()) {
        auto& v = j.at("value");
        math::Vector3 val;
        if (v.contains("x") && v.at("x").is_number()) val.x = v.at("x").get<float>();
        if (v.contains("y") && v.at("y").is_number()) val.y = v.at("y").get<float>();
        if (v.contains("z") && v.at("z").is_number()) val.z = v.at("z").get<float>();
        a.value = val;
    }
    if (j.contains("keyframes") && j.at("keyframes").is_array()) {
        a.keyframes = j.at("keyframes").get<std::vector<AnimatedVector3Spec::Keyframe>>();
    }
    if (j.contains("expression") && j.at("expression").is_string()) a.expression = j.at("expression").get<std::string>();
}

// MaskPathKeyframeSpec serialization
void to_json(json& j, const MaskPathKeyframeSpec& k) {
    j["time"] = k.time;
    json vertices_array = json::array();
    for (const auto& v : k.value.vertices) {
        json v_obj;
        v_obj["position"] = json{{"x", v.position.x}, {"y", v.position.y}};
        v_obj["in_tangent"] = json{{"x", v.in_tangent.x}, {"y", v.in_tangent.y}};
        v_obj["out_tangent"] = json{{"x", v.out_tangent.x}, {"y", v.out_tangent.y}};
        v_obj["feather_inner"] = v.feather_inner;
        v_obj["feather_outer"] = v.feather_outer;
        vertices_array.push_back(v_obj);
    }
    j["value"] = json{
        {"vertices", vertices_array},
        {"is_closed", k.value.is_closed},
        {"is_inverted", k.value.is_inverted}
    };
    j["easing"] = static_cast<int>(k.easing);
    j["bezier"] = json{{"cx1", k.bezier.cx1}, {"cy1", k.bezier.cy1}, {"cx2", k.bezier.cx2}, {"cy2", k.bezier.cy2}};
    j["speed_in"] = k.speed_in;
    j["influence_in"] = k.influence_in;
    j["speed_out"] = k.speed_out;
    j["influence_out"] = k.influence_out;
}

void from_json(const json& j, MaskPathKeyframeSpec& k) {
    if (j.contains("time") && j.at("time").is_number()) k.time = j.at("time").get<double>();
    if (j.contains("value") && j.at("value").is_object()) {
        auto& v = j.at("value");
        if (v.contains("vertices") && v.at("vertices").is_array()) {
            for (auto& vert : v.at("vertices")) {
                renderer2d::MaskVertex mv;
                if (vert.contains("position") && vert.at("position").is_object()) {
                    auto& p = vert.at("position");
                    if (p.contains("x") && p.at("x").is_number()) mv.position.x = p.at("x").get<float>();
                    if (p.contains("y") && p.at("y").is_number()) mv.position.y = p.at("y").get<float>();
                }
                if (vert.contains("in_tangent") && vert.at("in_tangent").is_object()) {
                    auto& t = vert.at("in_tangent");
                    if (t.contains("x") && t.at("x").is_number()) mv.in_tangent.x = t.at("x").get<float>();
                    if (t.contains("y") && t.at("y").is_number()) mv.in_tangent.y = t.at("y").get<float>();
                }
                if (vert.contains("out_tangent") && vert.at("out_tangent").is_object()) {
                    auto& t = vert.at("out_tangent");
                    if (t.contains("x") && t.at("x").is_number()) mv.out_tangent.x = t.at("x").get<float>();
                    if (t.contains("y") && t.at("y").is_number()) mv.out_tangent.y = t.at("y").get<float>();
                }
                if (vert.contains("feather_inner") && vert.at("feather_inner").is_number()) mv.feather_inner = vert.at("feather_inner").get<float>();
                if (vert.contains("feather_outer") && vert.at("feather_outer").is_number()) mv.feather_outer = vert.at("feather_outer").get<float>();
                k.value.vertices.push_back(mv);
            }
        }
        if (v.contains("is_closed") && v.at("is_closed").is_boolean()) k.value.is_closed = v.at("is_closed").get<bool>();
        if (v.contains("is_inverted") && v.at("is_inverted").is_boolean()) k.value.is_inverted = v.at("is_inverted").get<bool>();
    }
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

// AnimatedMaskPathSpec serialization
void to_json(json& j, const AnimatedMaskPathSpec& a) {
    if (a.value.has_value()) {
        json vertices_array = json::array();
        for (const auto& v : a.value.value().vertices) {
            json v_obj;
            v_obj["position"] = json{{"x", v.position.x}, {"y", v.position.y}};
            v_obj["in_tangent"] = json{{"x", v.in_tangent.x}, {"y", v.in_tangent.y}};
            v_obj["out_tangent"] = json{{"x", v.out_tangent.x}, {"y", v.out_tangent.y}};
            v_obj["feather_inner"] = v.feather_inner;
            v_obj["feather_outer"] = v.feather_outer;
            vertices_array.push_back(v_obj);
        }
        j["value"] = json{{"vertices", vertices_array}, {"is_closed", a.value.value().is_closed}, {"is_inverted", a.value.value().is_inverted}};
    }
    if (!a.keyframes.empty()) j["keyframes"] = a.keyframes;
}

void from_json(const json& j, AnimatedMaskPathSpec& a) {
    if (j.contains("value") && j.at("value").is_object()) {
        renderer2d::MaskPath mp;
        auto& v = j.at("value");
        if (v.contains("vertices") && v.at("vertices").is_array()) {
            for (auto& vert : v.at("vertices")) {
                renderer2d::MaskVertex mv;
                if (vert.contains("position") && vert.at("position").is_object()) {
                    auto& p = vert.at("position");
                    if (p.contains("x") && p.at("x").is_number()) mv.position.x = p.at("x").get<float>();
                    if (p.contains("y") && p.at("y").is_number()) mv.position.y = p.at("y").get<float>();
                }
                if (vert.contains("in_tangent") && vert.at("in_tangent").is_object()) {
                    auto& t = vert.at("in_tangent");
                    if (t.contains("x") && t.at("x").is_number()) mv.in_tangent.x = t.at("x").get<float>();
                    if (t.contains("y") && t.at("y").is_number()) mv.in_tangent.y = t.at("y").get<float>();
                }
                if (vert.contains("out_tangent") && vert.at("out_tangent").is_object()) {
                    auto& t = vert.at("out_tangent");
                    if (t.contains("x") && t.at("x").is_number()) mv.out_tangent.x = t.at("x").get<float>();
                    if (t.contains("y") && t.at("y").is_number()) mv.out_tangent.y = t.at("y").get<float>();
                }
                if (vert.contains("feather_inner") && vert.at("feather_inner").is_number()) mv.feather_inner = vert.at("feather_inner").get<float>();
                if (vert.contains("feather_outer") && vert.at("feather_outer").is_number()) mv.feather_outer = vert.at("feather_outer").get<float>();
                mp.vertices.push_back(mv);
            }
        }
        if (v.contains("is_closed") && v.at("is_closed").is_boolean()) mp.is_closed = v.at("is_closed").get<bool>();
        if (v.contains("is_inverted") && v.at("is_inverted").is_boolean()) mp.is_inverted = v.at("is_inverted").get<bool>();
        a.value = mp;
    }
    if (j.contains("keyframes") && j.at("keyframes").is_array()) {
        a.keyframes = j.at("keyframes").get<std::vector<MaskPathKeyframeSpec>>();
    }
}

} // namespace tachyon
