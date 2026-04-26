#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace tachyon {

// JSON serialization for ProceduralSpec
void to_json(json& j, const ProceduralSpec& p) {
    j["kind"] = p.kind;
    if (!p.frequency.empty()) j["frequency"] = p.frequency;
    if (!p.speed.empty()) j["speed"] = p.speed;
    if (!p.amplitude.empty()) j["amplitude"] = p.amplitude;
    if (!p.scale.empty()) j["scale"] = p.scale;
    if (!p.color_a.empty()) j["color_a"] = p.color_a;
    if (!p.color_b.empty()) j["color_b"] = p.color_b;
    if (p.seed != 0) j["seed"] = p.seed;
    if (!p.angle.empty()) j["angle"] = p.angle;
    if (!p.spacing.empty()) j["spacing"] = p.spacing;
}

void from_json(const json& j, ProceduralSpec& p) {
    if (j.contains("kind") && j.at("kind").is_string()) p.kind = j.at("kind").get<std::string>();
    if (j.contains("frequency")) p.frequency = j.at("frequency").get<AnimatedScalarSpec>();
    if (j.contains("speed")) p.speed = j.at("speed").get<AnimatedScalarSpec>();
    if (j.contains("amplitude")) p.amplitude = j.at("amplitude").get<AnimatedScalarSpec>();
    if (j.contains("scale")) p.scale = j.at("scale").get<AnimatedScalarSpec>();
    if (j.contains("color_a")) p.color_a = j.at("color_a").get<AnimatedColorSpec>();
    if (j.contains("color_b")) p.color_b = j.at("color_b").get<AnimatedColorSpec>();
    if (j.contains("seed") && j.at("seed").is_number()) p.seed = j.at("seed").get<uint64_t>();
    if (j.contains("angle")) p.angle = j.at("angle").get<AnimatedScalarSpec>();
    if (j.contains("spacing")) p.spacing = j.at("spacing").get<AnimatedScalarSpec>();
}

// JSON serialization for ParticleSpec
void to_json(json& j, const ParticleSpec& p) {
    j["emitter_shape"] = p.emitter_shape;
    if (!p.emission_rate.empty()) j["emission_rate"] = p.emission_rate;
    if (!p.lifetime.empty()) j["lifetime"] = p.lifetime;
    if (!p.start_speed.empty()) j["start_speed"] = p.start_speed;
    if (!p.direction.empty()) j["direction"] = p.direction;
    if (!p.spread.empty()) j["spread"] = p.spread;
    if (!p.gravity.empty()) j["gravity"] = p.gravity;
    if (!p.drag.empty()) j["drag"] = p.drag;
    if (!p.turbulence.empty()) j["turbulence"] = p.turbulence;
    if (!p.start_size.empty()) j["start_size"] = p.start_size;
    if (!p.end_size.empty()) j["end_size"] = p.end_size;
    if (!p.start_color.empty()) j["start_color"] = p.start_color;
    if (!p.end_color.empty()) j["end_color"] = p.end_color;
    if (p.blend_mode != "normal") j["blend_mode"] = p.blend_mode;
}

void from_json(const json& j, ParticleSpec& p) {
    if (j.contains("emitter_shape") && j.at("emitter_shape").is_string()) p.emitter_shape = j.at("emitter_shape").get<std::string>();
    if (j.contains("emission_rate")) p.emission_rate = j.at("emission_rate").get<AnimatedScalarSpec>();
    if (j.contains("lifetime")) p.lifetime = j.at("lifetime").get<AnimatedScalarSpec>();
    if (j.contains("start_speed")) p.start_speed = j.at("start_speed").get<AnimatedScalarSpec>();
    if (j.contains("direction")) p.direction = j.at("direction").get<AnimatedScalarSpec>();
    if (j.contains("spread")) p.spread = j.at("spread").get<AnimatedScalarSpec>();
    if (j.contains("gravity")) p.gravity = j.at("gravity").get<AnimatedScalarSpec>();
    if (j.contains("drag")) p.drag = j.at("drag").get<AnimatedScalarSpec>();
    if (j.contains("turbulence")) p.turbulence = j.at("turbulence").get<AnimatedScalarSpec>();
    if (j.contains("start_size")) p.start_size = j.at("start_size").get<AnimatedScalarSpec>();
    if (j.contains("end_size")) p.end_size = j.at("end_size").get<AnimatedScalarSpec>();
    if (j.contains("start_color")) p.start_color = j.at("start_color").get<AnimatedColorSpec>();
    if (j.contains("end_color")) p.end_color = j.at("end_color").get<AnimatedColorSpec>();
    if (j.contains("blend_mode") && j.at("blend_mode").is_string()) p.blend_mode = j.at("blend_mode").get<std::string>();
}

} // namespace tachyon
