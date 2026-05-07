#pragma once
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <string>
#include <optional>

namespace tachyon::presets {

/**
 * @brief Unified resolver for instantiating scenes from presets.
 * Consolidates logic from CLI and Catalog to ensure consistent behavior.
 */
class PresetSceneResolver {
public:
    /**
     * @brief Instantiates a SceneSpec from a preset ID.
     * 1. Tries ScenePresetRegistry (Full scenes)
     * 2. Tries BackgroundPresetRegistry (Single layer scenes)
     */
    static std::optional<SceneSpec> instantiate(const std::string& preset_id);

    /**
     * @brief Checks if a preset ID exists in any registry.
     */
    static bool exists(const std::string& preset_id);
};

} // namespace tachyon::presets
