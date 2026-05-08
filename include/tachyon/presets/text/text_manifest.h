#pragma once
 
#include <vector>
 
namespace tachyon::presets {
 
struct TextLayerPresetSpec;
struct TextAnimatorPresetSpec;
 
/**
 * @brief Manifest that consolidates all text preset generation.
 * 
 * This is the single canonical source for both text layer presets
 * and text animator presets.
 */
class TextManifest {
public:
    /**
     * @brief Generate all text layer preset specs.
     * @return Vector of all text layer preset specs.
     */
    std::vector<TextLayerPresetSpec> generate_layer_preset_specs() const;
 
    /**
     * @brief Generate all text animator preset specs.
     * @return Vector of all text animator preset specs.
     */
    std::vector<TextAnimatorPresetSpec> generate_animator_preset_specs() const;
};
 
} // namespace tachyon::presets
