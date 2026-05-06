#include "tachyon/presets/text/text_animator_preset_registry.h"
#include "tachyon/text/animation/text_presets.h"

namespace tachyon::presets {

TextAnimatorPresetRegistry& TextAnimatorPresetRegistry::instance() {
    static TextAnimatorPresetRegistry registry;
    return registry;
}

TextAnimatorPresetRegistry::TextAnimatorPresetRegistry() {
    load_builtins();
}

void TextAnimatorPresetRegistry::register_spec(TextAnimatorPresetSpec spec) {
    registry_.register_spec(std::move(spec));
}

const TextAnimatorPresetSpec* TextAnimatorPresetRegistry::find(std::string_view id) const {
    return registry_.find(id);
}

std::vector<TextAnimatorSpec> TextAnimatorPresetRegistry::create(std::string_view id, const registry::ParameterBag& params) const {
    if (const auto* spec = find(id)) {
        return spec->factory(params);
    }

    return {};
}

std::vector<std::string> TextAnimatorPresetRegistry::list_ids() const {
    return registry_.list_ids();
}

} // namespace tachyon::presets
