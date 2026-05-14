#pragma once

#include "tachyon/core/api.h"
#include "tachyon/core/spec/schema/effects/effect_spec.h"

namespace tachyon::scene {

class LayerBuilder;

/**
 * @brief Builder for effects on a LayerSpec.
 */
class TACHYON_API EffectBuilder {
    LayerBuilder& parent_;
public:
    explicit EffectBuilder(LayerBuilder& parent) 
        : parent_(parent) {}

    EffectBuilder& add(EffectSpec effect);

    LayerBuilder& done();
};

} // namespace tachyon::scene
