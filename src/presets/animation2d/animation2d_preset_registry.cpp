#include "tachyon/presets/animation2d/animation2d_preset_registry.h"

namespace tachyon::presets {

Animation2DPresetRegistry& Animation2DPresetRegistry::instance() {
    static Animation2DPresetRegistry registry;
    return registry;
}

Animation2DPresetRegistry::Animation2DPresetRegistry() {
    load_builtins();
}

void Animation2DPresetRegistry::register_spec(Animation2DPresetSpec spec) {
    registry_.register_spec(std::move(spec));
}

const Animation2DPresetSpec* Animation2DPresetRegistry::find(std::string_view id) const {
    return registry_.find(id);
}

bool Animation2DPresetRegistry::apply(std::string_view id, LayerSpec& layer, const Animation2DParams& params) const {
    if (const auto* spec = find(id)) {
        spec->apply(layer, params);
        return true;
    }
    return false;
}

std::vector<std::string> Animation2DPresetRegistry::list_ids() const {
    return registry_.list_ids();
}

} // namespace tachyon::presets
