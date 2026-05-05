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
    
    // Default fallback
    std::string based_on = params.get_or<std::string>("based_on", "characters_excluding_spaces");
    double stagger_delay = params.get_or<double>("stagger_delay", 0.03);
    double reveal_duration = params.get_or<double>("reveal_duration", 0.5);
    return std::vector<TextAnimatorSpec>{tachyon::text::make_fade_in_animator(based_on, stagger_delay, reveal_duration)};
}

std::vector<std::string> TextAnimatorPresetRegistry::list_ids() const {
    return registry_.list_ids();
}

} // namespace tachyon::presets
