#pragma once

#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <vector>
#include <optional>

namespace tachyon::presets {

/**
 * @brief Specification for a background preset.
 */
struct BackgroundPresetSpec {
    std::string id;
    std::string name;
    std::string description;
    
    // Factory function to create the background layer.
    // Takes width, height and optional duration.
    std::function<LayerSpec(int width, int height, double duration)> factory;
};

/**
 * @brief Registry for background presets.
 * Centralizes all procedural background generation logic.
 */
class BackgroundPresetRegistry {
public:
    static BackgroundPresetRegistry& instance() {
        static BackgroundPresetRegistry registry;
        return registry;
    }

    // Registers a new background preset.
    void register_preset(BackgroundPresetSpec spec);

    // Finds a preset by ID.
    const BackgroundPresetSpec* find(std::string_view id) const;

    // Creates a background layer using the specified ID.
    std::optional<LayerSpec> create(std::string_view id, int width, int height, double duration = 8.0) const {
        if (const auto* spec = find(id)) {
            auto layer = spec->factory(width, height, duration);
            layer.id = "bg_" + std::string(id);
            layer.name = spec->name;
            layer.preset_id = std::string(id);
            return layer;
        }
        return std::nullopt;
    }

    // Lists all registered background IDs.
    std::vector<std::string> list_ids() const;

    // Loads built-in presets from the table.
    void load_builtins();

private:
    BackgroundPresetRegistry();
    ~BackgroundPresetRegistry();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

/**
 * Compatibility wrapper functions.
 */
std::optional<LayerSpec> build_background_preset(std::string_view id, int width, int height);
std::vector<std::string> list_background_presets();

} // namespace tachyon::presets
