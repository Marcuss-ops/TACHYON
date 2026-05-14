#include "tachyon/presets/effects/effect_preset_registry.h"
#include <stdexcept>
#include <iterator>

namespace tachyon::presets {

std::vector<EffectPresetSpec> get_blur_effect_preset_specs();
std::vector<EffectPresetSpec> get_color_effect_preset_specs();
std::vector<EffectPresetSpec> get_distortion_effect_preset_specs();
std::vector<EffectPresetSpec> get_generator_effect_preset_specs();
std::vector<EffectPresetSpec> get_stylize_effect_preset_specs();
std::vector<EffectPresetSpec> get_transition_effect_preset_specs();

namespace {

std::vector<EffectPresetSpec> collect_builtin_preset_specs() {
    std::vector<EffectPresetSpec> specs;

    auto append = [](std::vector<EffectPresetSpec>& into, std::vector<EffectPresetSpec> from) {
        into.insert(into.end(),
            std::make_move_iterator(from.begin()),
            std::make_move_iterator(from.end()));
    };

    append(specs, get_blur_effect_preset_specs());
    append(specs, get_color_effect_preset_specs());
    append(specs, get_distortion_effect_preset_specs());
    append(specs, get_generator_effect_preset_specs());
    append(specs, get_stylize_effect_preset_specs());
    append(specs, get_transition_effect_preset_specs());

    // Load data-driven presets
    load_presets_from_directory("assets/library/effects", specs);

    return specs;
}

} // namespace

const EffectPresetRegistry& EffectPresetRegistry::instance() {
    static const EffectPresetRegistry registry;
    return registry;
}

EffectPresetRegistry::EffectPresetRegistry() {
    auto specs = collect_builtin_preset_specs();
    for (auto& spec : specs) {
        register_spec(std::move(spec));
    }
}

EffectSpec EffectPresetRegistry::create(std::string_view id, const registry::ParameterBag& params) const {
    if (const auto* spec = find(id)) {
        return spec->factory(params);
    }

    throw std::runtime_error("Unknown effect '" + std::string(id) + "'");
}

} // namespace tachyon::presets
