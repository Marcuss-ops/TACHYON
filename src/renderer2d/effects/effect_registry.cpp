#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/presets/effects/effect_specs.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_fast_path_registry.h"
#include "tachyon/renderer2d/effects/core/transitions/transition_fast_paths.h"
#include <unordered_map>

#include <iterator>
#include <algorithm>

namespace tachyon::renderer2d {

std::vector<EffectImplementation> get_blur_effect_implementations();
std::vector<EffectImplementation> get_color_effect_implementations();
std::vector<EffectImplementation> get_transition_effect_implementations(const tachyon::TransitionRegistry& transition_registry);
std::vector<EffectImplementation> get_distortion_effect_implementations();
std::vector<EffectImplementation> get_generator_effect_implementations();
std::vector<EffectImplementation> get_stylize_effect_implementations();

EffectRegistry::EffectRegistry() {
}

void EffectRegistry::register_spec(EffectDescriptor descriptor) {
    if (descriptor.id.empty()) {
        return;
    }
    registry_.register_spec(std::move(descriptor));
}

const EffectDescriptor* EffectRegistry::find(std::string_view id) const {
    return registry_.find(id);
}

std::vector<std::string> EffectRegistry::list_ids() const {
    return registry_.list_ids();
}

namespace {

std::vector<::tachyon::presets::EffectKindSpec> collect_builtin_kind_specs() {
    std::vector<::tachyon::presets::EffectKindSpec> specs;

    auto append = [](std::vector<::tachyon::presets::EffectKindSpec>& into, std::vector<::tachyon::presets::EffectKindSpec> from) {
        into.insert(into.end(),
            std::make_move_iterator(from.begin()),
            std::make_move_iterator(from.end()));
    };

    append(specs, ::tachyon::presets::get_blur_effect_kind_specs());
    append(specs, ::tachyon::presets::get_color_effect_kind_specs());
    append(specs, ::tachyon::presets::get_distortion_effect_kind_specs());
    append(specs, ::tachyon::presets::get_generator_effect_kind_specs());
    append(specs, ::tachyon::presets::get_stylize_effect_kind_specs());
    append(specs, ::tachyon::presets::get_transition_effect_kind_specs());
    return specs;
}

} // namespace

void register_builtin_effects(EffectRegistry& registry, const TransitionRegistry& transition_registry) {
    // 0. Register fast-path handler for Core to resolve circular dependency
    core::transition::TransitionFastPathRegistry::set_handler(apply_transition_fast_path);

    // 1. Collect all implementations

    std::vector<EffectImplementation> all_impls;
    
    auto blur = get_blur_effect_implementations();
    auto color = get_color_effect_implementations();
    auto distortion = get_distortion_effect_implementations();
    auto generator = get_generator_effect_implementations();
    auto stylize = get_stylize_effect_implementations();
    auto transitions = get_transition_effect_implementations(transition_registry);
    
    // Note: Transition effects are handled via kind specs but implementations 
    // are still gathered here for the join.
    // For now we assume they don't need the transition registry for the factory itself.
    // If they do, we'll need to pass it in.
    
    auto move_impls = [](std::vector<EffectImplementation>& from, std::vector<EffectImplementation>& to) {
        std::move(from.begin(), from.end(), std::back_inserter(to));
    };
    
    move_impls(blur, all_impls);
    move_impls(color, all_impls);
    move_impls(distortion, all_impls);
    move_impls(generator, all_impls);
    move_impls(stylize, all_impls);
    move_impls(transitions, all_impls);
    
    // Map implementations by ID
    std::unordered_map<std::string_view, EffectDescriptor::Factory> impl_map;
    for (const auto& impl : all_impls) {
        impl_map[impl.id] = impl.factory;
    }
    
    // 2. Join with Kind Specs from presets
    (void)transition_registry;
    auto kind_specs = collect_builtin_kind_specs();
    for (auto& kind : kind_specs) {
        auto it = impl_map.find(kind.id);
        if (it != impl_map.end()) {
            EffectDescriptor desc;
            desc.id = std::move(kind.id);
            desc.metadata = std::move(kind.metadata);
            desc.schema = std::move(kind.schema);
            desc.factory = it->second;
            registry.register_spec(std::move(desc));
        }
    }
}

} // namespace tachyon::renderer2d
