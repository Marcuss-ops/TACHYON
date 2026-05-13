#include "tachyon/scene/effect_builder.h"
#include "tachyon/scene/builder.h"
#include <stdexcept>

namespace tachyon::scene {

EffectBuilder& EffectBuilder::preset(std::string_view id, const registry::ParameterBag& params) {
    try {
        parent_.spec_.effects.push_back(preset_registry_.create(id, params));
    } catch (const std::exception& e) {
        throw std::runtime_error(
            "Layer '" + parent_.spec_.id + "': " + e.what()
        );
    }
    return *this;
}

LayerBuilder& EffectBuilder::done() {
    return parent_;
}

} // namespace tachyon::scene
