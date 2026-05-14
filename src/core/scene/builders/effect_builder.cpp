#include "tachyon/scene/effect_builder.h"
#include "tachyon/scene/builder.h"
#include <stdexcept>

namespace tachyon::scene {

EffectBuilder& EffectBuilder::add(EffectSpec effect) {
    parent_.spec_.effects.push_back(std::move(effect));
    return *this;
}

LayerBuilder& EffectBuilder::done() {
    return parent_;
}

} // namespace tachyon::scene
