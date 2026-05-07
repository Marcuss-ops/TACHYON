#include "tachyon/scene/effect_builder.h"
#include "tachyon/scene/builder.h"

namespace tachyon::scene {

EffectBuilder& EffectBuilder::add(const std::string& id,
                                   const std::map<std::string, double>& scalars,
                                   const std::map<std::string, std::string>& strings) {
    EffectSpec e;
    e.type = id;
    e.enabled = true;
    for (const auto& [k, v] : scalars) {
        e.scalars[k] = v;
    }
    for (const auto& [k, v] : strings) {
        e.strings[k] = v;
    }
    parent_.spec_.effects.push_back(std::move(e));
    return *this;
}

EffectBuilder& EffectBuilder::light_leak(LightLeakPreset preset, float progress, float speed, int seed) {
    EffectSpec leak;
    leak.type = "light_leak";
    leak.scalars["preset"] = static_cast<double>(static_cast<int>(preset));
    leak.scalars["progress"] = static_cast<double>(progress);
    leak.scalars["speed"] = static_cast<double>(speed);
    leak.scalars["seed"] = static_cast<double>(seed);
    parent_.spec_.effects.push_back(leak);
    return *this;
}

LayerBuilder& EffectBuilder::done() {
    return parent_;
}

} // namespace tachyon::scene
