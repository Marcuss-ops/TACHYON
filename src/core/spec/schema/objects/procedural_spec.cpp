#include "tachyon/core/spec/schema/objects/procedural_spec.h"
#include "tachyon/core/spec/property_spec_serialize_helpers.h"

namespace tachyon {

void to_json(nlohmann::json& j, const ProceduralSpec& spec) {
    j = nlohmann::json{
        {"kind", spec.kind},
        {"seed", spec.seed},
        {"shape", spec.shape}
    };

    if (!spec.frequency.empty()) to_json(j["frequency"], spec.frequency);
    if (!spec.speed.empty()) to_json(j["speed"], spec.speed);
    if (!spec.amplitude.empty()) to_json(j["amplitude"], spec.amplitude);
    if (!spec.scale.empty()) to_json(j["scale"], spec.scale);
    if (!spec.angle.empty()) to_json(j["angle"], spec.angle);

    if (!spec.color_a.empty()) to_json(j["color_a"], spec.color_a);
    if (!spec.color_b.empty()) to_json(j["color_b"], spec.color_b);
    if (!spec.color_c.empty()) to_json(j["color_c"], spec.color_c);

    if (!spec.spacing.empty()) to_json(j["spacing"], spec.spacing);
    if (!spec.border_width.empty()) to_json(j["border_width"], spec.border_width);
    if (!spec.border_color.empty()) to_json(j["border_color"], spec.border_color);

    if (!spec.warp_strength.empty()) to_json(j["warp_strength"], spec.warp_strength);
    if (!spec.warp_frequency.empty()) to_json(j["warp_frequency"], spec.warp_frequency);
    if (!spec.warp_speed.empty()) to_json(j["warp_speed"], spec.warp_speed);

    if (!spec.grain_amount.empty()) to_json(j["grain_amount"], spec.grain_amount);
    if (!spec.grain_scale.empty()) to_json(j["grain_scale"], spec.grain_scale);
    if (!spec.scanline_intensity.empty()) to_json(j["scanline_intensity"], spec.scanline_intensity);
    if (!spec.scanline_frequency.empty()) to_json(j["scanline_frequency"], spec.scanline_frequency);
    if (!spec.contrast.empty()) to_json(j["contrast"], spec.contrast);
    if (!spec.gamma.empty()) to_json(j["gamma"], spec.gamma);
    if (!spec.saturation.empty()) to_json(j["saturation"], spec.saturation);
    if (!spec.softness.empty()) to_json(j["softness"], spec.softness);

    if (!spec.octave_decay.empty()) to_json(j["octave_decay"], spec.octave_decay);
    if (!spec.band_height.empty()) to_json(j["band_height"], spec.band_height);
    if (!spec.band_spread.empty()) to_json(j["band_spread"], spec.band_spread);
}

void from_json(const nlohmann::json& j, ProceduralSpec& spec) {
    if (j.contains("kind") && j.at("kind").is_string()) spec.kind = j.at("kind").get<std::string>();
    if (j.contains("seed") && j.at("seed").is_number()) spec.seed = j.at("seed").get<uint64_t>();
    if (j.contains("shape") && j.at("shape").is_string()) spec.shape = j.at("shape").get<std::string>();

    if (j.contains("frequency")) spec.frequency = j.at("frequency").get<AnimatedScalarSpec>();
    if (j.contains("speed")) spec.speed = j.at("speed").get<AnimatedScalarSpec>();
    if (j.contains("amplitude")) spec.amplitude = j.at("amplitude").get<AnimatedScalarSpec>();
    if (j.contains("scale")) spec.scale = j.at("scale").get<AnimatedScalarSpec>();
    if (j.contains("angle")) spec.angle = j.at("angle").get<AnimatedScalarSpec>();

    if (j.contains("color_a")) spec.color_a = j.at("color_a").get<AnimatedColorSpec>();
    if (j.contains("color_b")) spec.color_b = j.at("color_b").get<AnimatedColorSpec>();
    if (j.contains("color_c")) spec.color_c = j.at("color_c").get<AnimatedColorSpec>();

    if (j.contains("spacing")) spec.spacing = j.at("spacing").get<AnimatedScalarSpec>();
    if (j.contains("border_width")) spec.border_width = j.at("border_width").get<AnimatedScalarSpec>();
    if (j.contains("border_color")) spec.border_color = j.at("border_color").get<AnimatedColorSpec>();

    if (j.contains("warp_strength")) spec.warp_strength = j.at("warp_strength").get<AnimatedScalarSpec>();
    if (j.contains("warp_frequency")) spec.warp_frequency = j.at("warp_frequency").get<AnimatedScalarSpec>();
    if (j.contains("warp_speed")) spec.warp_speed = j.at("warp_speed").get<AnimatedScalarSpec>();

    if (j.contains("grain_amount")) spec.grain_amount = j.at("grain_amount").get<AnimatedScalarSpec>();
    if (j.contains("grain_scale")) spec.grain_scale = j.at("grain_scale").get<AnimatedScalarSpec>();
    if (j.contains("scanline_intensity")) spec.scanline_intensity = j.at("scanline_intensity").get<AnimatedScalarSpec>();
    if (j.contains("scanline_frequency")) spec.scanline_frequency = j.at("scanline_frequency").get<AnimatedScalarSpec>();
    if (j.contains("contrast")) spec.contrast = j.at("contrast").get<AnimatedScalarSpec>();
    if (j.contains("gamma")) spec.gamma = j.at("gamma").get<AnimatedScalarSpec>();
    if (j.contains("saturation")) spec.saturation = j.at("saturation").get<AnimatedScalarSpec>();
    if (j.contains("softness")) spec.softness = j.at("softness").get<AnimatedScalarSpec>();

    if (j.contains("octave_decay")) spec.octave_decay = j.at("octave_decay").get<AnimatedScalarSpec>();
    if (j.contains("band_height")) spec.band_height = j.at("band_height").get<AnimatedScalarSpec>();
    if (j.contains("band_spread")) spec.band_spread = j.at("band_spread").get<AnimatedScalarSpec>();
}

} // namespace tachyon
