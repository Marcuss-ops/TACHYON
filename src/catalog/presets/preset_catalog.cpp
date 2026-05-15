#include "tachyon/catalog/presets/preset_catalog.h"
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>

namespace tachyon::content {

struct PresetCatalog::Impl {
    mutable std::mutex mutex;
    std::unordered_map<std::string, PresetEntry> entries_map;
    std::unordered_map<std::string, AliasEntry> aliases_map;

    std::unordered_map<std::string, std::function<std::vector<tachyon::TextAnimatorSpec>(const registry::ParameterBag&)>> text_animator_factories;
    std::unordered_map<std::string, std::function<tachyon::LayerSpec(const registry::ParameterBag&)>> background_factories;
    std::unordered_map<std::string, std::function<tachyon::spec::AudioTrackSpec(const registry::ParameterBag&)>> sfx_factories;
};

PresetCatalog& PresetCatalog::instance() {
    static PresetCatalog instance;
    return instance;
}

PresetCatalog::PresetCatalog() : m_impl(std::make_unique<Impl>()) {}
PresetCatalog::~PresetCatalog() = default;

void PresetCatalog::register_entry(PresetEntry entry) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::string id_str = entry.id;
    m_impl->entries_map[id_str] = std::move(entry);
}

void PresetCatalog::register_text_animator(std::string_view id, std::function<std::vector<tachyon::TextAnimatorSpec>(const registry::ParameterBag&)> factory) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->text_animator_factories[std::string(id)] = std::move(factory);
}

void PresetCatalog::register_background(std::string_view id, std::function<tachyon::LayerSpec(const registry::ParameterBag&)> factory) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->background_factories[std::string(id)] = std::move(factory);
}

void PresetCatalog::register_sfx(std::string_view id, std::function<tachyon::spec::AudioTrackSpec(const registry::ParameterBag&)> factory) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->sfx_factories[std::string(id)] = std::move(factory);
}

const PresetEntry* PresetCatalog::find(std::string_view id) const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto it = m_impl->entries_map.find(std::string(id));
    if (it != m_impl->entries_map.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<std::string> PresetCatalog::list_ids(std::optional<ContentKind> kind) const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::vector<std::string> ids;
    for (const auto& pair : m_impl->entries_map) {
        if (!kind || pair.second.kind == *kind) {
            ids.push_back(pair.first);
        }
    }
    return ids;
}

void PresetCatalog::register_alias(std::string alias_id, std::string target_id, registry::ParameterBag default_overrides) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    AliasEntry alias;
    alias.target_id = std::move(target_id);
    alias.default_overrides = std::move(default_overrides);
    m_impl->aliases_map[alias_id] = std::move(alias);
}

PresetCatalog::ResolutionResult PresetCatalog::resolve(std::string_view id, const registry::ParameterBag& user_params) const {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    
    std::string current_id(id);
    registry::ParameterBag merged = user_params;
    
    // Resolve aliases
    auto alias_it = m_impl->aliases_map.find(current_id);
    if (alias_it != m_impl->aliases_map.end()) {
        current_id = alias_it->second.target_id;
        
        registry::ParameterBag new_merged = alias_it->second.default_overrides;
        std::vector<std::string> keys = user_params.list_keys();
        for (const auto& key : keys) {
            if (auto val = user_params.get_raw(key)) {
                new_merged.set_raw(key, *val);
            }
        }
        merged = std::move(new_merged);
    }
    
    auto entry_it = m_impl->entries_map.find(current_id);
    ResolutionResult result;
    if (entry_it != m_impl->entries_map.end()) {
        result.entry = &entry_it->second;
    }
    result.resolved_id = current_id;
    result.merged_params = std::move(merged);
    
    return result;
}

std::optional<std::vector<tachyon::TextAnimatorSpec>> PresetCatalog::create_text_animator(std::string_view id, const registry::ParameterBag& params) const {
    auto res = resolve(id, params);
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto it = m_impl->text_animator_factories.find(res.resolved_id);
    if (it != m_impl->text_animator_factories.end()) {
        return it->second(res.merged_params);
    }
    return std::nullopt;
}

std::optional<tachyon::LayerSpec> PresetCatalog::create_background(std::string_view id, const registry::ParameterBag& params) const {
    auto res = resolve(id, params);
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto it = m_impl->background_factories.find(res.resolved_id);
    if (it != m_impl->background_factories.end()) {
        return it->second(res.merged_params);
    }
    return std::nullopt;
}

std::optional<tachyon::spec::AudioTrackSpec> PresetCatalog::create_sfx(std::string_view id, const registry::ParameterBag& params) const {
    auto res = resolve(id, params);
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto it = m_impl->sfx_factories.find(res.resolved_id);
    if (it != m_impl->sfx_factories.end()) {
        return it->second(res.merged_params);
    }
    return std::nullopt;
}

} // namespace tachyon::content
