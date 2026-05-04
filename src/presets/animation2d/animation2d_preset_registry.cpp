#include "tachyon/presets/animation2d/animation2d_preset_registry.h"
#include <map>
#include <mutex>

namespace tachyon::presets {

struct Animation2DPresetRegistry::Impl {
    std::map<std::string, Animation2DPresetSpec, std::less<>> presets;
    std::mutex mutex;
};

Animation2DPresetRegistry& Animation2DPresetRegistry::instance() {
    static Animation2DPresetRegistry registry;
    return registry;
}

Animation2DPresetRegistry::Animation2DPresetRegistry() : m_impl(std::make_unique<Impl>()) {
    load_builtins();
}

void Animation2DPresetRegistry::register_preset(Animation2DPresetSpec spec) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->presets[spec.id] = std::move(spec);
}

const Animation2DPresetSpec* Animation2DPresetRegistry::find(std::string_view id) const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto it = m_impl->presets.find(id);
    if (it != m_impl->presets.end()) {
        return &it->second;
    }
    return nullptr;
}

bool Animation2DPresetRegistry::apply(std::string_view id, LayerSpec& layer, const Animation2DParams& params) const {
    if (const auto* spec = find(id)) {
        spec->apply(layer, params);
        return true;
    }
    return false;
}

std::vector<std::string> Animation2DPresetRegistry::list_ids() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::vector<std::string> ids;
    ids.reserve(m_impl->presets.size());
    for (const auto& [id, _] : m_impl->presets) {
        ids.push_back(id);
    }
    return ids;
}

} // namespace tachyon::presets
