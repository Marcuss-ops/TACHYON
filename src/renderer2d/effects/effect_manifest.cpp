#include "tachyon/renderer2d/effects/effect_manifest.h"
#include <unordered_map>
#include <iterator>

namespace tachyon::renderer2d {

// External implementation tables
std::vector<EffectImplementation> get_blur_effect_implementations();
std::vector<EffectImplementation> get_color_effect_implementations();
std::vector<EffectImplementation> get_transition_effect_implementations(const tachyon::TransitionRegistry& transition_registry);
std::vector<EffectImplementation> get_distortion_effect_implementations();
std::vector<EffectImplementation> get_generator_effect_implementations();
std::vector<EffectImplementation> get_stylize_effect_implementations();

EffectManifest::EffectManifest(const TransitionRegistry& transition_registry)
    : transition_registry_(transition_registry) {
}

std::vector<EffectDescriptor> EffectManifest::generate_descriptors() const {
    // 1. Collect all implementations
    std::vector<EffectImplementation> all_impls;
    
    auto blur = get_blur_effect_implementations();
    auto color = get_color_effect_implementations();
    auto transition = get_transition_effect_implementations(transition_registry_);
    auto distortion = get_distortion_effect_implementations();
    auto generator = get_generator_effect_implementations();
    auto stylize = get_stylize_effect_implementations();
    
    auto move_impls = [](std::vector<EffectImplementation>& from, std::vector<EffectImplementation>& to) {
        std::move(from.begin(), from.end(), std::back_inserter(to));
    };
    
    move_impls(blur, all_impls);
    move_impls(color, all_impls);
    move_impls(transition, all_impls);
    move_impls(distortion, all_impls);
    move_impls(generator, all_impls);
    move_impls(stylize, all_impls);
    
    // Map implementations by ID for fast lookup
    std::unordered_map<std::string_view, EffectDescriptor::Factory> impl_map;
    for (const auto& impl : all_impls) {
        impl_map[impl.id] = impl.factory;
    }
    
    // 2. Get all kind specs from presets (Source of Truth)
    auto kind_specs = presets_manifest_.generate_kind_specs();
    
    // 3. Join them into descriptors
    std::vector<EffectDescriptor> descriptors;
    descriptors.reserve(kind_specs.size());
    
    for (auto& kind : kind_specs) {
        auto it = impl_map.find(kind.id);
        if (it != impl_map.end()) {
            descriptors.push_back({
                std::move(kind.id),
                std::move(kind.metadata),
                std::move(kind.schema),
                it->second
            });
        }
    }
    
    return descriptors;
}

} // namespace tachyon::renderer2d
