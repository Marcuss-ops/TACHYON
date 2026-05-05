#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/renderer2d/effects/core/effect_host.h"

namespace tachyon::renderer2d {

namespace {

template <typename T>
void register_builtin(
    EffectRegistry& registry,
    std::string id,
    std::string name,
    std::string category) {
    registry.register_spec({
        id,
        {id, name, "Professional color effect.", "effect." + category, {"color", category}},
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            T effect;
            output = effect.apply(input, params);
        }
    });
}

} // namespace

void register_color_effects(EffectRegistry& registry) {
    register_builtin<LevelsEffect>(registry, "tachyon.effect.color.levels", "Levels", "color");
    register_builtin<CurvesEffect>(registry, "tachyon.effect.color.curves", "Curves", "color");
    register_builtin<FillEffect>(registry, "tachyon.effect.color.fill", "Fill", "color");
    register_builtin<TintEffect>(registry, "tachyon.effect.color.tint", "Tint", "color");
    register_builtin<HueSaturationEffect>(registry, "tachyon.effect.color.hue_saturation", "Hue/Saturation", "color");
    register_builtin<ColorBalanceEffect>(registry, "tachyon.effect.color.balance", "Color Balance", "color");
    register_builtin<LUTEffect>(registry, "tachyon.effect.color.lut", "LUT", "color");
    register_builtin<Lut3DCubeEffect>(registry, "tachyon.effect.color.lut3d", "3D LUT (.cube)", "color");
}

} // namespace tachyon::renderer2d
