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

std::vector<TextAnimatorSpec> TextAnimatorPresetRegistry::create(std::string_view id,
                                                                 const std::string& based_on,
                                                                 double stagger_delay,
                                                                 double reveal_duration) const {
    if (const auto* spec = find(id)) {
        return spec->factory(based_on, stagger_delay, reveal_duration);
    }
    
    // Default fallback
    return std::vector<TextAnimatorSpec>{tachyon::text::make_fade_in_animator(based_on, stagger_delay, reveal_duration)};
}

std::vector<std::string> TextAnimatorPresetRegistry::list_ids() const {
    return registry_.list_ids();
}

} // namespace tachyon::presets
