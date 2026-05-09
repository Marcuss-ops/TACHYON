#include "tachyon/scene/builder.h"

namespace tachyon::scene {

namespace {

const presets::EffectPresetRegistry& default_effect_registry() {
    static const presets::EffectPresetRegistry registry;
    return registry;
}

} // namespace

// SceneBuilder implementation
SceneBuilder::SceneBuilder()
    : preset_registry_(default_effect_registry()) {
    spec_.project.id = "new_project";
    spec_.project.name = "New Project";
}

SceneBuilder::SceneBuilder(std::string id, std::string name, const presets::EffectPresetRegistry& preset_registry)
    : preset_registry_(preset_registry) {
    spec_.project.id = std::move(id);
    spec_.project.name = std::move(name);
}

SceneBuilder& SceneBuilder::project(std::string id, std::string name) {
    spec_.project.id = std::move(id);
    spec_.project.name = std::move(name);
    return *this;
}

SceneBuilder& SceneBuilder::composition(std::string id, std::function<void(CompositionBuilder&)> fn) {
    CompositionBuilder cb(std::move(id), preset_registry_);
    fn(cb);
    spec_.compositions.push_back(std::move(cb).build());
    return *this;
}

SceneSpec SceneBuilder::build() {
    return std::move(spec_);
}

CompositionBuilder Composition(std::string id, const presets::EffectPresetRegistry& preset_registry) {
    return CompositionBuilder(std::move(id), preset_registry);
}

CompositionBuilder Composition(std::string id) {
    return CompositionBuilder(std::move(id), default_effect_registry());
}

SceneBuilder Scene(const presets::EffectPresetRegistry& preset_registry) {
    return SceneBuilder("new_project", "New Project", preset_registry);
}

SceneBuilder Scene() {
    return SceneBuilder();
}

} // namespace tachyon::scene
