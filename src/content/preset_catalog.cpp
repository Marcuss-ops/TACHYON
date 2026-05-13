#include "tachyon/content/preset_catalog.h"
#include <unordered_map>
#include <mutex>
#include <string>
#include <vector>

namespace tachyon::content {

struct PresetCatalog::Impl {
    mutable std::mutex mutex;
    std::unordered_map<std::string, PresetEntry> entries_map;
    std::unordered_map<std::string, AliasEntry> aliases_map;
};

PresetCatalog& PresetCatalog::instance() {
    static PresetCatalog instance;
    return instance;
}

PresetCatalog::PresetCatalog() : m_impl(std::make_unique<Impl>()) {}
PresetCatalog::~PresetCatalog() = default;

void PresetCatalog::register_preset(PresetEntry entry) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    std::string id_str = entry.id;
    m_impl->entries_map[id_str] = std::move(entry);
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
        result.merged_params = std::move(merged);
    } else {
        result.entry = nullptr;
        result.merged_params = merged;
    }
    return result;
}

} // namespace tachyon::content
