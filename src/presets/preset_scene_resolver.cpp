#include "tachyon/presets/preset_scene_resolver.h"
#include "tachyon/presets/scene/scene_preset_registry.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/scene/builder.h"

namespace tachyon::presets {

std::optional<SceneSpec> PresetSceneResolver::instantiate(const std::string& preset_id) {
    // 1. Try Scene Preset Registry (Modern/Multi-layer)
    if (auto scene = ScenePresetRegistry::instance().create(preset_id, {})) {
        return std::move(*scene);
    }

    // 2. Fallback to Background Preset Registry (Legacy/Single Layer)
    presets::BackgroundPresetRegistry bg_registry;
    auto bg = bg_registry.create(preset_id, {});
    if (bg) {
        return ::tachyon::scene::Composition("preset_render")
            .size(1280, 720)
            .duration(2.0)
            .fps(30)
            .layer("bg", [&](::tachyon::scene::LayerBuilder& l) {
                l = ::tachyon::scene::LayerBuilder(*bg);
            })
            .build_scene();
    }

    return std::nullopt;
}

bool PresetSceneResolver::exists(const std::string& preset_id) {
    if (ScenePresetRegistry::instance().find(preset_id)) return true;
    presets::BackgroundPresetRegistry bg_registry;
    if (bg_registry.find(preset_id)) return true;
    return false;
}

} // namespace tachyon::presets
