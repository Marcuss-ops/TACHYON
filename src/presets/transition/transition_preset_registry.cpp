#include "tachyon/presets/transition/transition_preset_registry.h"

namespace tachyon::presets {

LayerTransitionSpec TransitionPresetRegistry::build_spec_from_params(const registry::ParameterBag& p, TransitionKind kind, const std::string& transition_id) {
    LayerTransitionSpec spec;
    spec.kind = kind;
    spec.type = transition_id;
    spec.transition_id = transition_id;
    spec.duration = p.get_or<double>("duration", 0.4);
    spec.easing = static_cast<animation::EasingPreset>(p.get_or<int>("easing", static_cast<int>(animation::EasingPreset::EaseOut)));
    spec.delay = p.get_or<double>("delay", 0.0);
    spec.direction = p.get_or<std::string>("direction", "none");
    return spec;
}

namespace {

LayerTransitionSpec make_noop_transition_from_params(const registry::ParameterBag& p) {
    return TransitionPresetRegistry::build_spec_from_params(p, TransitionKind::None, p.get_or<std::string>("id", "none"));
}

} // namespace

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

} // namespace tachyon::presets
