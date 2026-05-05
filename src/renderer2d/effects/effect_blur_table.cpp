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
        {id, name, "Professional blur effect.", "effect." + category, {"blur", category}},
        [](const EffectSpec&, const SurfaceRGBA& input, SurfaceRGBA& output, const std::vector<const SurfaceRGBA*>&, const EffectParams& params) {
            T effect;
            output = effect.apply(input, params);
        }
    });
}

} // namespace

void register_blur_effects(EffectRegistry& registry) {
    register_builtin<GaussianBlurEffect>(registry, "tachyon.effect.blur.gaussian", "Gaussian Blur", "blur");
    register_builtin<DirectionalBlurEffect>(registry, "tachyon.effect.blur.directional", "Directional Blur", "blur");
    register_builtin<RadialBlurEffect>(registry, "tachyon.effect.blur.radial", "Radial Blur", "blur");
    register_builtin<VectorBlurEffect>(registry, "tachyon.effect.blur.vector", "Vector Blur", "blur");
}

} // namespace tachyon::renderer2d
