#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"

namespace tachyon::renderer2d {

void register_generator_effects(EffectRegistry& registry) {
    // Light Leak
    EffectDescriptor light_leak_desc;
    light_leak_desc.id = "tachyon.effect.generator.light_leak";
    light_leak_desc.metadata = {
        light_leak_desc.id,
        "Light Leak",
        "Procedural light leak and film burn effect.",
        "effect.generator",
        {"light", "film", "vintage"}
    };
    light_leak_desc.schema = registry::ParameterSchema({
        {"progress", "Progress", "Animation progress (0-1)", 0.0, 0.0, 1.0},
        {"speed", "Speed", "Animation speed multiplier", 1.0, 0.1, 10.0},
        {"seed", "Seed", "Random seed", 3.0},
        {"preset", "Preset", "Preset type (0-N)", 0.0},
        {"intensity", "Intensity", "Light intensity", 1.0, 0.0, 5.0},
        {"width", "Width", "Leak width", 0.5, 0.0, 2.0},
        {"color_a", "Color A", "Start color", ColorSpec{255, 204, 153, 255}},
        {"color_b", "Color B", "End color", ColorSpec{255, 102, 51, 255}}
    });
    light_leak_desc.factory = [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
        LightLeakEffect effect;
        output = effect.apply(input, params);
    };
    registry.register_spec(std::move(light_leak_desc));

    // Particle Emitter
    EffectDescriptor particle_desc;
    particle_desc.id = "tachyon.effect.generator.particle_emitter";
    particle_desc.metadata = {
        particle_desc.id,
        "Particle Emitter",
        "Simple procedural particle system.",
        "effect.generator",
        {"particle", "emitter", "physics"}
    };
    particle_desc.schema = registry::ParameterSchema({
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
    });
    particle_desc.factory = [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
        ParticleEmitterEffect effect;
        output = effect.apply(input, params);
    };
    registry.register_spec(std::move(particle_desc));

    // Lens Flare
    EffectDescriptor lens_flare_desc;
    lens_flare_desc.id = "tachyon.effect.generator.lens_flare";
    lens_flare_desc.metadata = {
        lens_flare_desc.id,
        "Lens Flare",
        "Simulated optical lens flare.",
        "effect.generator",
        {"lens", "flare", "optical"}
    };
    lens_flare_desc.schema = registry::ParameterSchema({
        {"lights", "Lights", "Encoded light configuration string", ""}
    });
    lens_flare_desc.factory = [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
        LensFlareEffect effect;
        output = effect.apply(input, params);
    };
    registry.register_spec(std::move(lens_flare_desc));
}

} // namespace tachyon::renderer2d
