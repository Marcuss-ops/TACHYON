#include "tachyon/renderer2d/effects/effect_manifest.h"
#include <iterator>
#include <vector>

namespace tachyon::renderer2d {

// Forward declarations for table descriptor functions
std::vector<EffectDescriptor> get_blur_effect_descriptors();
std::vector<EffectDescriptor> get_color_effect_descriptors();
std::vector<EffectDescriptor> get_transition_effect_descriptors(const tachyon::TransitionRegistry& transition_registry);
std::vector<EffectDescriptor> get_distortion_effect_descriptors();
std::vector<EffectDescriptor> get_generator_effect_descriptors();
std::vector<EffectDescriptor> get_stylize_effect_descriptors();

// Forward declarations for preset spec functions
std::vector<presets::EffectPresetSpec> get_blur_effect_preset_specs();
std::vector<presets::EffectPresetSpec> get_color_effect_preset_specs();
std::vector<presets::EffectPresetSpec> get_transition_effect_preset_specs();
std::vector<presets::EffectPresetSpec> get_distortion_effect_preset_specs();
std::vector<presets::EffectPresetSpec> get_generator_effect_preset_specs();
std::vector<presets::EffectPresetSpec> get_stylize_effect_preset_specs();

EffectManifest::EffectManifest(const TransitionRegistry& transition_registry)
    : transition_registry_(transition_registry) {
}

std::vector<EffectDescriptor> EffectManifest::generate_descriptors() const {
    std::vector<EffectDescriptor> all_descriptors;
    
    // Collect descriptors from all categories
    auto blur_descriptors = get_blur_effect_descriptors();
    auto color_descriptors = get_color_effect_descriptors();
    auto transition_descriptors = get_transition_effect_descriptors(transition_registry_);
    auto distortion_descriptors = get_distortion_effect_descriptors();
    auto generator_descriptors = get_generator_effect_descriptors();
    auto stylize_descriptors = get_stylize_effect_descriptors();
    
    // Reserve total capacity to avoid reallocations
    all_descriptors.reserve(
        blur_descriptors.size() +
        color_descriptors.size() +
        transition_descriptors.size() +
        distortion_descriptors.size() +
        generator_descriptors.size() +
        stylize_descriptors.size());
    
    // Move all descriptors into the result vector
    auto move_descriptors = [](std::vector<EffectDescriptor>& from, std::vector<EffectDescriptor>& to) {
        std::move(from.begin(), from.end(), std::back_inserter(to));
    };
    
    move_descriptors(blur_descriptors, all_descriptors);
    move_descriptors(color_descriptors, all_descriptors);
    move_descriptors(transition_descriptors, all_descriptors);
    move_descriptors(distortion_descriptors, all_descriptors);
    move_descriptors(generator_descriptors, all_descriptors);
    move_descriptors(stylize_descriptors, all_descriptors);
    
    return all_descriptors;
}

std::vector<presets::EffectPresetSpec> EffectManifest::generate_preset_specs() const {
    std::vector<presets::EffectPresetSpec> all_specs;
    
    // Collect preset specs from all categories
    auto blur_specs = get_blur_effect_preset_specs();
    auto color_specs = get_color_effect_preset_specs();
    auto transition_specs = get_transition_effect_preset_specs();
    auto distortion_specs = get_distortion_effect_preset_specs();
    auto generator_specs = get_generator_effect_preset_specs();
    auto stylize_specs = get_stylize_effect_preset_specs();
    
    // Reserve total capacity
    all_specs.reserve(
        blur_specs.size() +
        color_specs.size() +
        transition_specs.size() +
        distortion_specs.size() +
        generator_specs.size() +
        stylize_specs.size());
    
    // Move all specs into the result vector
    auto move_specs = [](std::vector<presets::EffectPresetSpec>& from, std::vector<presets::EffectPresetSpec>& to) {
        std::move(from.begin(), from.end(), std::back_inserter(to));
    };
    
    move_specs(blur_specs, all_specs);
    move_specs(color_specs, all_specs);
    move_specs(transition_specs, all_specs);
    move_specs(distortion_specs, all_specs);
    move_specs(generator_specs, all_specs);
    move_specs(stylize_specs, all_specs);
    
    return all_specs;
}

} // namespace tachyon::renderer2d
