#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/renderer2d/effects/generators/light_leak_types.h"

#include <string>
#include <map>

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

    EffectBuilder& add(const std::string& id,
                      const std::map<std::string, double>& scalars = {},
                      const std::map<std::string, std::string>& strings = {});
    EffectBuilder& light_leak(LightLeakPreset preset, float progress, float speed = 1.0f, int seed = 3);

    LayerBuilder& done();
};

} // namespace tachyon::scene
