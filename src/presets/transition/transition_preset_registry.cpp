#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

namespace {

LayerTransitionSpec make_noop_transition_from_params(const registry::ParameterBag& p) {
    LayerTransitionSpec spec;
    spec.transition_id = p.get_or<std::string>("id", "");
    spec.type          = "none";
    spec.kind      = TransitionKind::None;
    spec.duration      = p.get_or<double>("duration", 0.4);
    spec.easing        = static_cast<animation::EasingPreset>(p.get_or<int>("easing", static_cast<int>(animation::EasingPreset::EaseOut)));
    spec.delay         = p.get_or<double>("delay", 0.0);
    spec.direction     = p.get_or<std::string>("direction", "none");
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

LayerTransitionSpec TransitionPresetRegistry::create(std::string_view id, const registry::ParameterBag& params) const {
    if (id.empty() || id == "none") {
        LayerTransitionSpec spec;
        spec.type = "none";
        spec.kind = TransitionKind::None;
        return spec;
    }

    if (const auto* spec = find(id)) {
        return spec->factory(params);
    }

    return make_noop_transition_from_params(params);
}

std::vector<std::string> TransitionPresetRegistry::list_ids() const {
    return registry_.list_ids();
}

} // namespace tachyon::presets
