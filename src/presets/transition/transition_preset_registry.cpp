#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

namespace {

LayerTransitionSpec make_basic_spec(const TransitionParams& p) {
    LayerTransitionSpec spec;
    spec.transition_id = p.id;
    spec.type          = p.id.empty() ? "none" : p.id;
    spec.duration      = p.duration;
    spec.easing        = p.easing;
    spec.delay         = p.delay;
    spec.direction     = p.direction;
    return spec;
}

} // namespace

TransitionPresetRegistry& TransitionPresetRegistry::instance() {
    static TransitionPresetRegistry registry;
    return registry;
}

TransitionPresetRegistry::TransitionPresetRegistry() {
    load_builtins();
}

void TransitionPresetRegistry::register_spec(TransitionPresetSpec spec) {
    registry_.register_spec(std::move(spec));
}

const TransitionPresetSpec* TransitionPresetRegistry::find(std::string_view id) const {
    return registry_.find(id);
}

LayerTransitionSpec TransitionPresetRegistry::create(std::string_view id, const TransitionParams& params) const {
    if (id.empty() || id == "none") {
        LayerTransitionSpec spec;
        spec.type = "none";
        return spec;
    }

    if (const auto* spec = find(id)) {
        return spec->factory(params);
    }
    
    return make_basic_spec(params);
}

std::vector<std::string> TransitionPresetRegistry::list_ids() const {
    return registry_.list_ids();
}

} // namespace tachyon::presets
