#include "tachyon/presets/scene/scene_preset_registry.h"
#include "tachyon/presets/scene/common.h"

namespace tachyon::presets {

ScenePresetRegistry& ScenePresetRegistry::instance() {
    static ScenePresetRegistry instance;
    return instance;
}

ScenePresetRegistry::ScenePresetRegistry() {
    load_builtins();
}

void ScenePresetRegistry::register_spec(ScenePresetSpec spec) {
    registry_.register_spec(std::move(spec));
}

const ScenePresetSpec* ScenePresetRegistry::find(std::string_view id) const {
    return registry_.find(id);
}

std::optional<SceneSpec> ScenePresetRegistry::create(std::string_view id, const registry::ParameterBag& params) const {
    if (const auto* spec = find(id)) {
        return spec->create(params);
    }
    return std::nullopt;
}

std::vector<std::string> ScenePresetRegistry::list_ids() const {
    return registry_.list_ids();
}

void ScenePresetRegistry::load_builtins() {
    register_spec({
        "tachyon.scene.enhanced_text",
        {"tachyon.scene.enhanced_text", "Modern Aura", "Clean animated text with modern styling.", "scene.built-in", {"text", "modern"}},
        {},
        [](const registry::ParameterBag&) { return scene::build_enhanced_text_scene(); }
    });
    
    register_spec({
        "tachyon.scene.modern_grid",
        {"tachyon.scene.modern_grid", "Modern Tech Grid", "High-tech grid background with floating elements.", "scene.built-in", {"grid", "tech"}},
        {},
        [](const registry::ParameterBag&) { return scene::build_modern_grid_scene(); }
    });

    register_spec({
        "tachyon.scene.classico_premium",
        {"tachyon.scene.classico_premium", "Classico Premium", "Elegant premium presentation style.", "scene.built-in", {"premium", "elegant"}},
        {},
        [](const registry::ParameterBag&) { return scene::build_classico_premium_scene(); }
    });

    register_spec({
        "tachyon.scene.minimal_text",
        {"tachyon.scene.minimal_text", "Minimal White", "Simple minimal white background with bold text.", "scene.built-in", {"minimal", "white"}},
        {},
        [](const registry::ParameterBag&) { return scene::build_minimal_text_scene(); }
    });

    register_spec({
        "tachyon.scene.a",
        {"tachyon.scene.a", "Scene A", "Blue background with centered text.", "scene.built-in", {"blue", "test"}},
        {},
        [](const registry::ParameterBag&) { return scene::build_scene_a(); }
    });

    register_spec({
        "tachyon.scene.b",
        {"tachyon.scene.b", "Scene B", "Red background with centered text.", "scene.built-in", {"red", "test"}},
        {},
        [](const registry::ParameterBag&) { return scene::build_scene_b(); }
    });
}

} // namespace tachyon::presets
