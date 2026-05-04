#include "tachyon/presets/background/background_preset_registry.h"
#include <map>
#include <mutex>

namespace tachyon::presets {

struct BackgroundPresetRegistry::Impl {
    std::map<std::string, BackgroundPresetSpec, std::less<>> presets;
    std::mutex mutex;
};


BackgroundPresetRegistry::BackgroundPresetRegistry() : m_impl(std::make_unique<Impl>()) {
    load_builtins();
}

BackgroundPresetRegistry::~BackgroundPresetRegistry() = default;

void BackgroundPresetRegistry::register_preset(BackgroundPresetSpec spec) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->presets[spec.id] = std::move(spec);
}

const BackgroundPresetSpec* BackgroundPresetRegistry::find(std::string_view id) const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto it = m_impl->presets.find(id);
    if (it != m_impl->presets.end()) {
        return &it->second;
    }
    return nullptr;
}


std::vector<std::string> BackgroundPresetRegistry::list_ids() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::vector<std::string> ids;
    ids.reserve(m_impl->presets.size());
    for (const auto& [id, _] : m_impl->presets) {
        ids.push_back(id);
    }
    return ids;
}

// Loads built-in presets from the table.
// Implementation is in background_presets_table.cpp
// void BackgroundPresetRegistry::load_builtins() { ... }

// Compatibility wrappers
std::optional<LayerSpec> build_background_preset(std::string_view id, int width, int height) {
    return BackgroundPresetRegistry::instance().create(id, width, height);
}

std::vector<std::string> list_background_presets() {
    return BackgroundPresetRegistry::instance().list_ids();
}

} // namespace tachyon::presets
