#include "tachyon/core/spec/schema/objects/procedural_spec.h"

namespace tachyon {

void to_json(nlohmann::json& j, const ProceduralSpec& spec) {
    j = nlohmann::json{
        {"kind", spec.kind},
        {"seed", spec.seed},
        {"shape", spec.shape}
    };

    if (!spec.frequency.empty()) j["frequency"] = spec.frequency;
    if (!spec.speed.empty()) j["speed"] = spec.speed;
    if (!spec.amplitude.empty()) j["amplitude"] = spec.amplitude;
    if (!spec.scale.empty()) j["scale"] = spec.scale;
    if (!spec.angle.empty()) j["angle"] = spec.angle;

    if (!spec.color_a.empty()) j["color_a"] = spec.color_a;
    if (!spec.color_b.empty()) j["color_b"] = spec.color_b;
    if (!spec.color_c.empty()) j["color_c"] = spec.color_c;

    if (!spec.spacing.empty()) j["spacing"] = spec.spacing;
    if (!spec.border_width.empty()) j["border_width"] = spec.border_width;
    if (!spec.border_color.empty()) j["border_color"] = spec.border_color;

    if (!spec.warp_strength.empty()) j["warp_strength"] = spec.warp_strength;
    if (!spec.warp_frequency.empty()) j["warp_frequency"] = spec.warp_frequency;
    if (!spec.warp_speed.empty()) j["warp_speed"] = spec.warp_speed;

    if (!spec.grain_amount.empty()) j["grain_amount"] = spec.grain_amount;
    if (!spec.grain_scale.empty()) j["grain_scale"] = spec.grain_scale;
    if (!spec.scanline_intensity.empty()) j["scanline_intensity"] = spec.scanline_intensity;
    if (!spec.scanline_frequency.empty()) j["scanline_frequency"] = spec.scanline_frequency;
    if (!spec.contrast.empty()) j["contrast"] = spec.contrast;
    if (!spec.gamma.empty()) j["gamma"] = spec.gamma;
    if (!spec.saturation.empty()) j["saturation"] = spec.saturation;
    if (!spec.softness.empty()) j["softness"] = spec.softness;

    if (!spec.octave_decay.empty()) j["octave_decay"] = spec.octave_decay;
    if (!spec.band_height.empty()) j["band_height"] = spec.band_height;
    if (!spec.band_spread.empty()) j["band_spread"] = spec.band_spread;
}

void from_json(const nlohmann::json& j, ProceduralSpec& spec) {
    if (j.contains("kind") && j.at("kind").is_string()) spec.kind = j.at("kind").get<std::string>();
    if (j.contains("seed") && j.at("seed").is_number()) spec.seed = j.at("seed").get<uint64_t>();
    if (j.contains("shape") && j.at("shape").is_string()) spec.shape = j.at("shape").get<std::string>();

    if (j.contains("frequency")) j.at("frequency").get_to(spec.frequency);
    if (j.contains("speed")) j.at("speed").get_to(spec.speed);
    if (j.contains("amplitude")) j.at("amplitude").get_to(spec.amplitude);
    if (j.contains("scale")) j.at("scale").get_to(spec.scale);
    if (j.contains("angle")) j.at("angle").get_to(spec.angle);

    if (j.contains("color_a")) j.at("color_a").get_to(spec.color_a);
    if (j.contains("color_b")) j.at("color_b").get_to(spec.color_b);
    if (j.contains("color_c")) j.at("color_c").get_to(spec.color_c);

    if (j.contains("spacing")) j.at("spacing").get_to(spec.spacing);
    if (j.contains("border_width")) j.at("border_width").get_to(spec.border_width);
    if (j.contains("border_color")) j.at("border_color").get_to(spec.border_color);

    if (j.contains("warp_strength")) j.at("warp_strength").get_to(spec.warp_strength);
    if (j.contains("warp_frequency")) j.at("warp_frequency").get_to(spec.warp_frequency);
    if (j.contains("warp_speed")) j.at("warp_speed").get_to(spec.warp_speed);

    if (j.contains("grain_amount")) j.at("grain_amount").get_to(spec.grain_amount);
    if (j.contains("grain_scale")) j.at("grain_scale").get_to(spec.grain_scale);
    if (j.contains("scanline_intensity")) j.at("scanline_intensity").get_to(spec.scanline_intensity);
    if (j.contains("scanline_frequency")) j.at("scanline_frequency").get_to(spec.scanline_frequency);
    if (j.contains("contrast")) j.at("contrast").get_to(spec.contrast);
    if (j.contains("gamma")) j.at("gamma").get_to(spec.gamma);
    if (j.contains("saturation")) j.at("saturation").get_to(spec.saturation);
    if (j.contains("softness")) j.at("softness").get_to(spec.softness);

    if (j.contains("octave_decay")) j.at("octave_decay").get_to(spec.octave_decay);
    if (j.contains("band_height")) j.at("band_height").get_to(spec.band_height);
    if (j.contains("band_spread")) j.at("band_spread").get_to(spec.band_spread);
}

} // namespace tachyon
