#include "tachyon/renderer2d/effects/effect_descriptor.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"

#include <array>

namespace tachyon::renderer2d {

namespace {

static const std::array<EffectBuiltinSpec, 3> kGeneratorEffects = {{
    {
        "tachyon.effect.generator.light_leak",
        "Light Leak",
        "generator",
        "Procedural light leak and film burn effect.",
        {"light", "film", "vintage"},
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
        make_effect_factory<LightLeakEffect>()
    },
    {
        "tachyon.effect.generator.particle_emitter",
        "Particle Emitter",
        "generator",
        "Simple procedural particle system.",
        {"particle", "emitter", "physics"},
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
        make_effect_factory<ParticleEmitterEffect>()
    },
    {
        "tachyon.effect.generator.lens_flare",
        "Lens Flare",
        "generator",
        "Simulated optical lens flare.",
        {"lens", "flare", "optical"},
        registry::ParameterSchema({
            {"lights", "Lights", "Encoded light configuration string", ""}
        }),
        make_effect_factory<LensFlareEffect>()
    }
}};

} // namespace

void register_generator_effects(EffectRegistry& registry) {
    for (const auto& spec : kGeneratorEffects) {
        register_effect_from_spec(registry, spec);
    }
}

} // namespace tachyon::renderer2d
