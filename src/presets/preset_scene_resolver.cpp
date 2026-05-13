#include "tachyon/presets/preset_scene_resolver.h"
#include "tachyon/presets/scene/scene_preset_registry.h"
#include "tachyon/content/preset_catalog.h"
#include "tachyon/scene/builder.h"
#include "tachyon/presets/effects/effect_preset_registry.h"

namespace tachyon::presets {

namespace {

std::optional<SceneSpec> wrap_background_as_scene(const LayerSpec& background_layer) {
    presets::EffectPresetRegistry effects;

    return ::tachyon::scene::Composition("preset_render", effects)
        .size(1280, 720)
        .duration(2.0)
        .fps(30)
        .layer(background_layer)
        .build_scene();
}

} // namespace

std::optional<SceneSpec> PresetSceneResolver::instantiate_scene_preset(const std::string& preset_id) {
    if (auto scene = ScenePresetRegistry::instance().create(preset_id, {})) {
        return std::move(*scene);
    }
    return std::nullopt;
}

std::optional<SceneSpec> PresetSceneResolver::instantiate_background_preset_as_scene(const std::string& preset_id) {
    auto bg = content::PresetCatalog::instance().create_background(preset_id, {});
    if (!bg) {
        return std::nullopt;
    }

    return wrap_background_as_scene(*bg);
}

std::optional<SceneSpec> PresetSceneResolver::instantiate_scene_or_background(const std::string& preset_id) {
    if (auto scene = instantiate_scene_preset(preset_id)) {
        return scene;
    }
    return instantiate_background_preset_as_scene(preset_id);
}

std::optional<SceneSpec> PresetSceneResolver::instantiate(const std::string& preset_id) {
    return instantiate_scene_or_background(preset_id);
}

bool PresetSceneResolver::exists_scene_preset(const std::string& preset_id) {
    return ScenePresetRegistry::instance().find(preset_id) != nullptr;
}

bool PresetSceneResolver::exists_background_preset(const std::string& preset_id) {
    auto entry = content::PresetCatalog::instance().find(preset_id);
    return entry != nullptr && entry->kind == content::ContentKind::Background;
}

bool PresetSceneResolver::exists(const std::string& preset_id) {
    return exists_scene_preset(preset_id) || exists_background_preset(preset_id);
}

} // namespace tachyon::presets
