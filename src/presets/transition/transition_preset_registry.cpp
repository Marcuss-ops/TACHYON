#include "tachyon/presets/transition/transition_preset_registry.h"
#include <map>
#include <mutex>

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

struct TransitionPresetRegistry::Impl {
    std::map<std::string, TransitionPresetSpec, std::less<>> presets;
    std::mutex mutex;
};

TransitionPresetRegistry& TransitionPresetRegistry::instance() {
    static TransitionPresetRegistry registry;
    return registry;
}

TransitionPresetRegistry::TransitionPresetRegistry() : m_impl(std::make_unique<Impl>()) {
    // Register default presets
    
    register_preset({
        "none",
        "None",
        "No transition",
        [](const TransitionParams& p) {
            LayerTransitionSpec spec;
            spec.type = "none";
            return spec;
        }
    });

    register_preset({
        "fade",
        "Fade",
        "Simple opacity fade",
        [](const TransitionParams& p) {
            return make_basic_spec(p);
        }
    });

    register_preset({
        "slide",
        "Slide",
        "Directional slide transition",
        [](const TransitionParams& p) {
            return make_basic_spec(p);
        }
    });

    register_preset({
        "zoom",
        "Zoom",
        "Scale-based zoom transition",
        [](const TransitionParams& p) {
            return make_basic_spec(p);
        }
    });

    // Example of a more complex preset with custom defaults
    register_preset({
        "elastic_slide",
        "Elastic Slide",
        "Slide with elastic spring easing",
        [](const TransitionParams& p) {
            auto spec = make_basic_spec(p);
            spec.easing = animation::EasingPreset::EaseOut; // Custom default for this preset
            spec.spring.stiffness = 400.0;
            spec.spring.damping = 15.0;
            return spec;
        }
    });
}

void TransitionPresetRegistry::register_preset(TransitionPresetSpec spec) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->presets[spec.id] = std::move(spec);
}

const TransitionPresetSpec* TransitionPresetRegistry::find(std::string_view id) const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto it = m_impl->presets.find(id);
    if (it != m_impl->presets.end()) {
        return &it->second;
    }
    return nullptr;
}

LayerTransitionSpec TransitionPresetRegistry::create_enter(const TransitionParams& params) const {
    if (params.id.empty()) {
        return create_enter({.id = "none"});
    }

    if (const auto* spec = find(params.id)) {
        return spec->factory(params);
    }
    
    // Fallback to basic spec if ID not found but provided
    return make_basic_spec(params);
}

LayerTransitionSpec TransitionPresetRegistry::create_exit(const TransitionParams& params) const {
    return create_enter(params); // Exit transitions often use same logic as enter
}

std::vector<std::string> TransitionPresetRegistry::list_ids() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::vector<std::string> ids;
    ids.reserve(m_impl->presets.size());
    for (const auto& [id, _] : m_impl->presets) {
        ids.push_back(id);
    }
    return ids;
}

} // namespace tachyon::presets
