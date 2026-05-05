#pragma once

#include "tachyon/core/registry/registry_metadata.h"
#include "tachyon/core/registry/typed_registry.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/presets/text/text_params.h"
#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <vector>
#include <optional>

namespace tachyon::presets {

/**
 * @brief Specification for a text layer style preset.
 */
struct TextLayerPresetSpec {
    std::string id;
    registry::RegistryMetadata metadata;
    
    // Factory function to create a text layer.
    std::function<LayerSpec(const std::string& content, const TextParams& p)> factory;
};

/**
 * @brief Registry for text layer style presets.
 */
class TextLayerPresetRegistry {
public:
    static TextLayerPresetRegistry& instance();

    void register_spec(TextLayerPresetSpec spec);
    const TextLayerPresetSpec* find(std::string_view id) const;
    
    std::optional<LayerSpec> create(std::string_view id, const std::string& content, const TextParams& p) const;

    std::vector<std::string> list_ids() const;
    void load_builtins();

private:
    TextLayerPresetRegistry();
    ~TextLayerPresetRegistry() = default;

    registry::TypedRegistry<TextLayerPresetSpec> registry_;
};

} // namespace tachyon::presets
