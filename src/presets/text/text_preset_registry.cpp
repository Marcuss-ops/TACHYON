#include "tachyon/presets/text/text_preset_registry.h"
#include <map>
#include <mutex>

namespace tachyon::presets {

struct TextPresetRegistry::Impl {
    std::map<std::string, TextPresetSpec, std::less<>> presets;
    std::mutex mutex;
};

TextPresetRegistry& TextPresetRegistry::instance() {
    static TextPresetRegistry registry;
    return registry;
}

TextPresetRegistry::TextPresetRegistry() : m_impl(std::make_unique<Impl>()) {
    load_builtins();
}

void TextPresetRegistry::register_preset(TextPresetSpec spec) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->presets[spec.id] = std::move(spec);
}

const TextPresetSpec* TextPresetRegistry::find(std::string_view id) const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto it = m_impl->presets.find(id);
    if (it != m_impl->presets.end()) {
        return &it->second;
    }
    return nullptr;
}

std::optional<LayerSpec> TextPresetRegistry::create(std::string_view id, const std::string& content, const TextParams& p) const {
    if (const auto* spec = find(id)) {
        auto layer = spec->factory(content, p);
        layer.preset_id = std::string(id);
        return layer;
    }
    return std::nullopt;
}

std::vector<std::string> TextPresetRegistry::list_ids() const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::vector<std::string> ids;
    ids.reserve(m_impl->presets.size());
    for (const auto& [id, _] : m_impl->presets) {
        ids.push_back(id);
    }
    return ids;
}

} // namespace tachyon::presets
