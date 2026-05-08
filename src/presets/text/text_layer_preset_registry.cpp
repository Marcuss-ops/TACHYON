#include "tachyon/presets/text/text_layer_preset_registry.h"

namespace tachyon::presets {

std::optional<LayerSpec> TextLayerPresetRegistry::create(std::string_view id, const registry::ParameterBag& params) const {
    if (const auto* spec = find(id)) {
        return spec->factory(params);
    }
    return std::nullopt;
}

void TextLayerPresetRegistry::load_from_manifest(const TextManifest& manifest) {
    auto specs = manifest.generate_layer_preset_specs();
    for (auto& spec : specs) {
        register_spec(std::move(spec));
    }
}

} // namespace tachyon::presets
