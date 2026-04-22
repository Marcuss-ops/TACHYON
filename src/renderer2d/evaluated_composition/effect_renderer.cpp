#include "tachyon/renderer2d/evaluated_composition/effect_renderer.h"
#include "composition_utils.h"
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

EffectParams effect_params_from_spec(const EffectSpec& spec) {
    EffectParams params;
    for (const auto& [key, value] : spec.scalars) {
        params.scalars.emplace(key, static_cast<float>(value));
    }
    for (const auto& [key, value] : spec.colors) {
        params.colors.emplace(key, detail::sRGB_to_Linear(Color{
            static_cast<float>(value.r) / 255.0f,
            static_cast<float>(value.g) / 255.0f,
            static_cast<float>(value.b) / 255.0f,
            static_cast<float>(value.a) / 255.0f
        }));
    }
    for (const auto& [key, value] : spec.strings) {
        params.strings.emplace(key, value);
    }
    return params;
}

SurfaceRGBA apply_effect_pipeline(
    const SurfaceRGBA& input,
    const std::vector<EffectSpec>& effects,
    EffectHost& host) {

    SurfaceRGBA current = input;
    for (const auto& effect : effects) {
        if (!effect.enabled || effect.type.empty()) {
            continue;
        }
        current = host.apply(effect.type, current, effect_params_from_spec(effect));
    }
    return current;
}

} // namespace tachyon::renderer2d
