#include "tachyon/presets/background/background_preset_registry.h"

namespace tachyon::presets {

std::optional<LayerSpec> BackgroundPresetRegistry::create(std::string_view id, const registry::ParameterBag& params) const {
    if (const auto* spec = find(id)) {
        auto layer = spec->factory(params);
        layer.id = "bg_" + std::string(id);
        layer.name = spec->metadata.display_name;
        layer.preset_id = std::string(id);
        
        if (layer.type == LayerType::NullLayer || !layer.enabled) {
            return std::nullopt;
        }
        return layer;
    }
    return std::nullopt;
}

} // namespace tachyon::presets
