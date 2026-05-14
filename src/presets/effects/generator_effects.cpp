#include "tachyon/presets/effects/effect_specs.h"
#include <vector>

namespace tachyon::presets {

std::vector<EffectKindSpec> get_generator_effect_kind_specs() {
    std::vector<EffectKindSpec> specs;

    // Light Leak
    specs.push_back({
        "tachyon.effect.generator.light_leak",
        {"tachyon.effect.generator.light_leak", "Light Leak", "Procedural light leak and film burn effect.", "effect.generator", {"light", "film", "vintage"}},
        registry::ParameterSchema({
            {"progress", "Progress", "Animation progress (0-1)", 0.0, 0.0, 1.0},
            {"speed", "Speed", "Animation speed multiplier", 1.0, 0.1, 10.0},
            {"seed", "Seed", "Random seed", 3.0},
            {"preset", "Preset", "Preset type (0-N)", 0.0},
            {"intensity", "Intensity", "Light intensity", 1.0, 0.0, 5.0},
            {"width", "Width", "Leak width", 0.5, 0.0, 2.0},
            {"color_a", "Color A", "Start color", ColorSpec{255, 204, 153, 255}},
            {"color_b", "Color B", "End color", ColorSpec{255, 102, 51, 255}}
        })
    });

    // Particle Emitter
    specs.push_back({
        "tachyon.effect.generator.particle_emitter",
        {"tachyon.effect.generator.particle_emitter", "Particle Emitter", "Simple procedural particle system.", "effect.generator", {"particle", "emitter", "physics"}},
        registry::ParameterSchema({
            {"time", "Time", "Current time in seconds", 0.0},
            {"seed", "Seed", "Random seed", 1.0},
            {"count", "Count", "Number of particles", 48.0, 1.0, 500.0},
            {"lifetime", "Lifetime", "Particle lifetime in seconds", 1.0, 0.1, 10.0},
            {"speed", "Speed", "Initial speed", 18.0},
            {"gravity", "Gravity", "Gravity force (Y-axis)", 0.0},
            {"spread_x", "Spread X", "Emission spread X", 1.0},
            {"spread_y", "Spread Y", "Emission spread Y", 1.0},
            {"radius_min", "Min Radius", "Minimum particle size", 1.0},
            {"radius_max", "Max Radius", "Maximum particle size", 3.0},
            {"color", "Color", "Particle color", ColorSpec{255, 255, 255, 166}},
            {"opacity", "Opacity", "Global opacity", 1.0, 0.0, 1.0},
            {"center_x", "Center X", "Emitter X position (normalized)", 0.5},
            {"center_y", "Center Y", "Emitter Y position (normalized)", 0.5},
            {"emit_width", "Emit Width", "Width of the emission area", 1920.0},
            {"emit_height", "Emit Height", "Height of the emission area", 1080.0}
        })
    });

    // Lens Flare
    specs.push_back({
        "tachyon.effect.generator.lens_flare",
        {"tachyon.effect.generator.lens_flare", "Lens Flare", "Simulated optical lens flare.", "effect.generator", {"lens", "flare", "optical"}},
        registry::ParameterSchema({
            {"lights", "Lights", "Encoded light configuration string", ""}
        })
    });

    return specs;
}

std::vector<EffectPresetSpec> get_generator_effect_preset_specs() {
    std::vector<EffectPresetSpec> specs;

    // Light Leak Preset
    specs.push_back({
        "tachyon.effect.generator.light_leak",
        {"tachyon.effect.generator.light_leak", "Light Leak", "Procedural light leak and film burn effect.", "effect.generator", {"light", "film", "vintage"}},
        registry::ParameterSchema({
            {"progress", "Progress", "Animation progress (0-1)", 0.0, 0.0, 1.0},
            {"speed", "Speed", "Animation speed multiplier", 1.0, 0.1, 10.0},
            {"seed", "Seed", "Random seed", 3.0},
            {"preset", "Preset", "Preset type (0-N)", 0.0},
            {"intensity", "Intensity", "Light intensity", 1.0, 0.0, 5.0},
            {"width", "Width", "Leak width", 0.5, 0.0, 2.0},
            {"color_a", "Color A", "Start color", ColorSpec{255, 204, 153, 255}},
            {"color_b", "Color B", "End color", ColorSpec{255, 102, 51, 255}}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.generator.light_leak";
            effect.params["progress"] = p.get_or<double>("progress", 0.0);
            effect.params["speed"] = p.get_or<double>("speed", 1.0);
            effect.params["seed"] = p.get_or<double>("seed", 3.0);
            effect.params["preset"] = p.get_or<double>("preset", 0.0);
            effect.params["intensity"] = p.get_or<double>("intensity", 1.0);
            effect.params["width"] = p.get_or<double>("width", 0.5);
            effect.params["color_a"] = p.get_or<ColorSpec>("color_a", ColorSpec{255, 204, 153, 255});
            effect.params["color_b"] = p.get_or<ColorSpec>("color_b", ColorSpec{255, 102, 51, 255});
            return effect;
        }
    });

    // Particle Emitter Preset
    specs.push_back({
        "tachyon.effect.generator.particle_emitter",
        {"tachyon.effect.generator.particle_emitter", "Particle Emitter", "Simple procedural particle system.", "effect.generator", {"particle", "emitter", "physics"}},
        registry::ParameterSchema({
            {"time", "Time", "Current time in seconds", 0.0},
            {"seed", "Seed", "Random seed", 1.0},
            {"count", "Count", "Number of particles", 48.0, 1.0, 500.0},
            {"lifetime", "Lifetime", "Particle lifetime in seconds", 1.0, 0.1, 10.0},
            {"speed", "Speed", "Initial speed", 18.0},
            {"gravity", "Gravity", "Gravity force (Y-axis)", 0.0},
            {"spread_x", "Spread X", "Emission spread X", 1.0},
            {"spread_y", "Spread Y", "Emission spread Y", 1.0},
            {"radius_min", "Min Radius", "Minimum particle size", 1.0},
            {"radius_max", "Max Radius", "Maximum particle size", 3.0},
            {"color", "Color", "Particle color", ColorSpec{255, 255, 255, 166}},
            {"opacity", "Opacity", "Global opacity", 1.0, 0.0, 1.0},
            {"center_x", "Center X", "Emitter X position (normalized)", 0.5},
            {"center_y", "Center Y", "Emitter Y position (normalized)", 0.5},
            {"emit_width", "Emit Width", "Width of the emission area", 1920.0},
            {"emit_height", "Emit Height", "Height of the emission area", 1080.0}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.generator.particle_emitter";
            effect.params["time"] = p.get_or<double>("time", 0.0);
            effect.params["seed"] = p.get_or<double>("seed", 1.0);
            effect.params["count"] = p.get_or<double>("count", 48.0);
            effect.params["lifetime"] = p.get_or<double>("lifetime", 1.0);
            effect.params["speed"] = p.get_or<double>("speed", 18.0);
            effect.params["gravity"] = p.get_or<double>("gravity", 0.0);
            effect.params["spread_x"] = p.get_or<double>("spread_x", 1.0);
            effect.params["spread_y"] = p.get_or<double>("spread_y", 1.0);
            effect.params["radius_min"] = p.get_or<double>("radius_min", 1.0);
            effect.params["radius_max"] = p.get_or<double>("radius_max", 3.0);
            effect.params["color"] = p.get_or<ColorSpec>("color", ColorSpec{255, 255, 255, 166});
            effect.params["opacity"] = p.get_or<double>("opacity", 1.0);
            effect.params["center_x"] = p.get_or<double>("center_x", 0.5);
            effect.params["center_y"] = p.get_or<double>("center_y", 0.5);
            effect.params["emit_width"] = p.get_or<double>("emit_width", 1920.0);
            effect.params["emit_height"] = p.get_or<double>("emit_height", 1080.0);
            return effect;
        }
    });

    // Lens Flare Preset
    specs.push_back({
        "tachyon.effect.generator.lens_flare",
        {"tachyon.effect.generator.lens_flare", "Lens Flare", "Simulated optical lens flare.", "effect.generator", {"lens", "flare", "optical"}},
        registry::ParameterSchema({
            {"lights", "Lights", "Encoded light configuration string", ""}
        }),
        [](const registry::ParameterBag& p) {
            EffectSpec effect;
            effect.type = "tachyon.effect.generator.lens_flare";
            effect.params["lights"] = p.get_or<std::string>("lights", "");
            return effect;
        }
    });

    return specs;
}

} // namespace tachyon::presets
