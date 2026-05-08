#include "tachyon/presets/animation2d/animation2d_preset_registry.h"

namespace tachyon::presets {

Animation2DPresetRegistry& Animation2DPresetRegistry::instance() {
    static Animation2DPresetRegistry instance;
    return instance;
}

bool Animation2DPresetRegistry::apply(std::string_view id, LayerSpec& layer, const registry::ParameterBag& params) const {
    if (const auto* spec = find(id)) {
        spec->apply(layer, params);
        return true;
    }
    return false;
}

} // namespace tachyon::presets
