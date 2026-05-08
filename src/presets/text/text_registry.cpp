#include "tachyon/presets/text/text_registry.h"

namespace tachyon::presets {

TextRegistry::TextRegistry(const TextManifest& manifest) {
    load_from_manifest(manifest);
}

std::optional<LayerSpec> TextRegistry::create_layer(std::string_view id, const registry::ParameterBag& params) const {
    if (const auto* spec = layers_.find(id)) {
        return spec->factory(params);
    }
    return std::nullopt;
}

std::vector<TextAnimatorSpec> TextRegistry::create_animators(std::string_view id, const registry::ParameterBag& params) const {
    if (const auto* spec = animators_.find(id)) {
        return spec->factory(params);
    }
    return {};
}

void TextRegistry::load_from_manifest(const TextManifest& manifest) {
    auto layer_specs = manifest.generate_layer_preset_specs();
    for (auto& spec : layer_specs) {
        layers_.register_spec(std::move(spec));
    }

    auto animator_specs = manifest.generate_animator_preset_specs();
    for (auto& spec : animator_specs) {
        animators_.register_spec(std::move(spec));
    }
}

} // namespace tachyon::presets
