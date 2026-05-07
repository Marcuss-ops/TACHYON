#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/registry/parameter_bag.h"
#include "tachyon/renderer2d/effects/generators/light_leak_types.h"
#include <string_view>

namespace tachyon::scene {

using renderer2d::LightLeakPreset;

class LayerBuilder;

/**
 * @brief Builder for effects on a LayerSpec.
 */
class TACHYON_API EffectBuilder {
    LayerBuilder& parent_;
public:
    explicit EffectBuilder(LayerBuilder& parent) : parent_(parent) {}

    EffectBuilder& light_leak(LightLeakPreset preset, float progress, float speed = 1.0f, int seed = 3);
    EffectBuilder& preset(std::string_view id, const registry::ParameterBag& params);

    LayerBuilder& done();
};

} // namespace tachyon::scene
