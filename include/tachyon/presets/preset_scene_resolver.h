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
     * @brief Instantiates a full scene preset.
     * Only ScenePresetRegistry is consulted.
     */
    static std::optional<SceneSpec> instantiate_scene_preset(const std::string& preset_id);

    /**
     * @brief Adapts a background preset into a one-composition SceneSpec.
     * This keeps background presets as an explicit adapter path.
     */
    static std::optional<SceneSpec> instantiate_background_preset_as_scene(const std::string& preset_id);

    /**
     * @brief Backward-compatible combined resolver.
     * Tries scene presets first, then the explicit background adapter.
     */
    static std::optional<SceneSpec> instantiate_scene_or_background(const std::string& preset_id);

    /**
     * @brief Backward-compatible alias for instantiate_scene_or_background.
     */
    static std::optional<SceneSpec> instantiate(const std::string& preset_id);

    /**
     * @brief Checks whether a full scene preset exists.
     */
    static bool exists_scene_preset(const std::string& preset_id);

    /**
     * @brief Checks whether a background preset exists.
     */
    static bool exists_background_preset(const std::string& preset_id);

    /**
     * @brief Checks if a preset ID exists in any supported preset registry.
     */
    static bool exists(const std::string& preset_id);
};

} // namespace tachyon::presets
