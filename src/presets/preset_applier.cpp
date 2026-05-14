#include "tachyon/presets/preset_applier.h"
#include "tachyon/presets/animation2d/animation2d_preset_registry.h"
#include "tachyon/presets/effects/effect_preset_registry.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/text/text_registry.h"
#include "tachyon/core/content/preset_catalog.h"
#include "tachyon/text/animation/text_animator_utils.h"

namespace tachyon::presets {

namespace {
void mark_fixed_pitch_if_needed(tachyon::LayerSpec& spec, const TextAnimatorSpec& anim) {
    if (::tachyon::text::uses_character_stagger_layout(anim)) {
        spec.text_box.fixed_pitch = true;
    }
}
}

void PresetApplier::apply_animation2d(LayerSpec& layer, const std::string& preset_id, const registry::ParameterBag& params) {
    Animation2DPresetRegistry::instance().apply(preset_id, layer, params);
}

void PresetApplier::apply_effect(LayerSpec& layer, const std::string& preset_id, const registry::ParameterBag& params) {
    layer.effects.push_back(EffectPresetRegistry::instance().create(preset_id, params));
}

void PresetApplier::apply_text_animation(LayerSpec& layer, const std::string& preset_id, const registry::ParameterBag& params) {
    presets::TextManifest text_manifest;
    presets::TextRegistry local_registry(text_manifest);
    auto anims = local_registry.create_animators(preset_id, params);
    for (const auto& anim : anims) {
        mark_fixed_pitch_if_needed(layer, anim);
    }
    layer.text_animators.insert(layer.text_animators.end(), anims.begin(), anims.end());
}

void PresetApplier::apply_background(CompositionSpec& comp, const std::string& preset_id, const registry::ParameterBag& params) {
    if (auto bg_layer = content::PresetCatalog::instance().create_background(preset_id, params)) {
        comp.layers.insert(comp.layers.begin(), std::move(*bg_layer));
    }
}

} // namespace tachyon::presets
