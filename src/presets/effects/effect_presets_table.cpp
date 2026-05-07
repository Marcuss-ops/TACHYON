#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::presets {

namespace {

void register_light_leak(EffectPresetRegistry& registry) {
    EffectPresetSpec spec;
    spec.id = "tachyon.effect.light_leak";
    spec.metadata = registry::RegistryMetadata{
        spec.id,
        "Light Leak",
        "Artistic light leak effect with various presets",
        "effect",
        {"artistic", "glsl"}
    };
    spec.schema = registry::ParameterSchema({
        {"preset", "Preset", "Light leak preset index", 0.0, 0.0, 10.0},
        {"progress", "Progress", "Effect progress (0-1)", 0.0, 0.0, 1.0},
        {"speed", "Speed", "Animation speed", 1.0, 0.0, 10.0},
        {"seed", "Seed", "Random seed", 3.0, 0.0, 1000.0}
    });
    spec.factory = [](const registry::ParameterBag& p) {
        EffectSpec effect;
        effect.type = "light_leak";
        effect.scalars["preset"] = p.get_or<double>("preset", 0.0);
        effect.scalars["progress"] = p.get_or<double>("progress", 0.0);
        effect.scalars["speed"] = p.get_or<double>("speed", 1.0);
        effect.scalars["seed"] = p.get_or<double>("seed", 3.0);
        return effect;
    };
    registry.register_spec(std::move(spec));
}

} // namespace

void EffectPresetRegistry::load_builtins() {
    register_light_leak(*this);
}

} // namespace tachyon::presets
