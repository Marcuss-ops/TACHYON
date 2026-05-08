#include "tachyon/presets/effects/effect_manifest.h"
#include <iterator>

namespace tachyon::presets {

// External category functions
std::vector<EffectKindSpec> get_blur_effect_kind_specs();
std::vector<EffectPresetSpec> get_blur_effect_preset_specs();

std::vector<EffectKindSpec> get_color_effect_kind_specs();
std::vector<EffectPresetSpec> get_color_effect_preset_specs();

std::vector<EffectKindSpec> get_distortion_effect_kind_specs();
std::vector<EffectPresetSpec> get_distortion_effect_preset_specs();

std::vector<EffectKindSpec> get_generator_effect_kind_specs();
std::vector<EffectPresetSpec> get_generator_effect_preset_specs();

std::vector<EffectKindSpec> get_stylize_effect_kind_specs();
std::vector<EffectPresetSpec> get_stylize_effect_preset_specs();

std::vector<EffectKindSpec> get_transition_effect_kind_specs();
std::vector<EffectPresetSpec> get_transition_effect_preset_specs();

std::vector<EffectKindSpec> EffectManifest::generate_kind_specs() const {
    std::vector<EffectKindSpec> all_specs;
    
    auto blur = get_blur_kinds();
    auto color = get_color_kinds();
    auto distort = get_distortion_kinds();
    auto generator = get_generator_kinds();
    auto stylize = get_stylize_kinds();
    auto transition = get_transition_kinds();
    
    auto move_specs = [](std::vector<EffectKindSpec>& from, std::vector<EffectKindSpec>& to) {
        std::move(from.begin(), from.end(), std::back_inserter(to));
    };
    
    move_specs(blur, all_specs);
    move_specs(color, all_specs);
    move_specs(distort, all_specs);
    move_specs(generator, all_specs);
    move_specs(stylize, all_specs);
    move_specs(transition, all_specs);
    
    return all_specs;
}

std::vector<EffectPresetSpec> EffectManifest::generate_preset_specs() const {
    std::vector<EffectPresetSpec> all_specs;
    
    auto blur = get_blur_presets();
    auto color = get_color_presets();
    auto distort = get_distortion_presets();
    auto generator = get_generator_presets();
    auto stylize = get_stylize_presets();
    auto transition = get_transition_presets();
    
    auto move_specs = [](std::vector<EffectPresetSpec>& from, std::vector<EffectPresetSpec>& to) {
        std::move(from.begin(), from.end(), std::back_inserter(to));
    };
    
    move_specs(blur, all_specs);
    move_specs(color, all_specs);
    move_specs(distort, all_specs);
    move_specs(generator, all_specs);
    move_specs(stylize, all_specs);
    move_specs(transition, all_specs);
    
    return all_specs;
}

std::vector<EffectKindSpec> EffectManifest::get_blur_kinds() const { return get_blur_effect_kind_specs(); }
std::vector<EffectKindSpec> EffectManifest::get_color_kinds() const { return get_color_effect_kind_specs(); }
std::vector<EffectKindSpec> EffectManifest::get_distortion_kinds() const { return get_distortion_effect_kind_specs(); }
std::vector<EffectKindSpec> EffectManifest::get_generator_kinds() const { return get_generator_effect_kind_specs(); }
std::vector<EffectKindSpec> EffectManifest::get_stylize_kinds() const { return get_stylize_effect_kind_specs(); }
std::vector<EffectKindSpec> EffectManifest::get_transition_kinds() const { return get_transition_effect_kind_specs(); }

std::vector<EffectPresetSpec> EffectManifest::get_blur_presets() const { return get_blur_effect_preset_specs(); }
std::vector<EffectPresetSpec> EffectManifest::get_color_presets() const { return get_color_effect_preset_specs(); }
std::vector<EffectPresetSpec> EffectManifest::get_distortion_presets() const { return get_distortion_effect_preset_specs(); }
std::vector<EffectPresetSpec> EffectManifest::get_generator_presets() const { return get_generator_effect_preset_specs(); }
std::vector<EffectPresetSpec> EffectManifest::get_stylize_presets() const { return get_stylize_effect_preset_specs(); }
std::vector<EffectPresetSpec> EffectManifest::get_transition_presets() const { return get_transition_effect_preset_specs(); }

} // namespace tachyon::presets
