#include "tachyon/renderer2d/evaluated_composition/effect_renderer.h"
#include "tachyon/renderer2d/evaluated_composition/utilities/composition_utils.h"
#include "tachyon/renderer2d/color/color_transfer.h"

namespace tachyon::renderer2d {

static EffectHost& builtin_effect_host() {
    static std::unique_ptr<EffectHost> host = [] {
        auto created = create_effect_host();
        EffectHost::register_builtins(*created);
        return created;
    }();
    return *host;
}

EffectHost& effect_host_for(RenderContext2D& context) {
    if (context.effects) {
        return *context.effects;
    }
    return builtin_effect_host();
}

EffectParams effect_params_from_spec(const EffectSpec& spec, const ColorProfile& working_profile) {
    EffectParams params;
    for (const auto& [key, value] : spec.scalars) {
        params.scalars.emplace(key, static_cast<float>(value));
    }
    for (const auto& [key, value] : spec.colors) {
        params.colors.emplace(key, from_color_spec(value, working_profile));
    }
    for (const auto& [key, value] : spec.strings) {
        params.strings.emplace(key, value);
    }
    return params;
}

EffectParams effect_params_from_spec(const EffectSpec& spec) {
    return effect_params_from_spec(spec, ColorProfile::Rec709_Linear());
}

SurfaceRGBA apply_effect_pipeline(
    const SurfaceRGBA& input,
    const std::vector<EffectSpec>& effects,
    EffectHost& host,
    const ColorProfile& working_profile) {

    SurfaceRGBA current = input;
    for (const auto& effect : effects) {
        if (!effect.enabled || effect.type.empty()) {
            continue;
        }
        current = host.apply(effect.type, current, effect_params_from_spec(effect, working_profile));
    }
    return current;
}

} // namespace tachyon::renderer2d
