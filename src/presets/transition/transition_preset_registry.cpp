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
    load_builtins();
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
        LayerTransitionSpec spec;
        spec.type = "none";
        return spec;
    }

    if (const auto* spec = find(params.id)) {
        return spec->factory(params);
    }
    
    // Strict mode would fail here. For now, we return a basic spec with type set to the ID
    // but the lack of registration will eventually trigger diagnostics in the renderer.
    return make_basic_spec(params);
}

LayerTransitionSpec TransitionPresetRegistry::create_exit(const TransitionParams& params) const {
    return create_enter(params); 
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
