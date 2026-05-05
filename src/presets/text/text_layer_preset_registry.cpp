#include "tachyon/presets/text/text_layer_preset_registry.h"

namespace tachyon::presets {

TextLayerPresetRegistry& TextLayerPresetRegistry::instance() {
    static TextLayerPresetRegistry registry;
    return registry;
}

TextLayerPresetRegistry::TextLayerPresetRegistry() {
    load_builtins();
}

void TextLayerPresetRegistry::register_spec(TextLayerPresetSpec spec) {
    registry_.register_spec(std::move(spec));
}

const TextLayerPresetSpec* TextLayerPresetRegistry::find(std::string_view id) const {
    return registry_.find(id);
}

std::optional<LayerSpec> TextLayerPresetRegistry::create(std::string_view id, const registry::ParameterBag& params) const {
    if (const auto* spec = find(id)) {
        return spec->factory(params);
    }
    return std::nullopt;
}

std::vector<std::string> TextLayerPresetRegistry::list_ids() const {
    return registry_.list_ids();
}

} // namespace tachyon::presets
