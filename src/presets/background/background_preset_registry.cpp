#include "tachyon/presets/background/background_preset_registry.h"

namespace tachyon::presets {

BackgroundPresetRegistry& BackgroundPresetRegistry::instance() {
    static BackgroundPresetRegistry registry;
    return registry;
}

BackgroundPresetRegistry::BackgroundPresetRegistry() {
    load_builtins();
}

void BackgroundPresetRegistry::register_spec(BackgroundPresetSpec spec) {
    registry_.register_spec(std::move(spec));
}

const BackgroundPresetSpec* BackgroundPresetRegistry::find(std::string_view id) const {
    return registry_.find(id);
}

std::optional<LayerSpec> BackgroundPresetRegistry::create(std::string_view id, const registry::ParameterBag& params) const {
    if (const auto* spec = find(id)) {
        auto layer = spec->factory(params);
        layer.id = "bg_" + std::string(id);
        layer.name = spec->metadata.display_name;
        layer.preset_id = std::string(id);
        // Background presets must resolve to normal composited layers, not disabled/null placeholders.
        if (layer.type == LayerType::NullLayer || !layer.enabled) {
            return std::nullopt;
        }
        return layer;
    }
    return std::nullopt;
}

std::vector<std::string> BackgroundPresetRegistry::list_ids() const {
    return registry_.list_ids();
}

} // namespace tachyon::presets
