#include "tachyon/transition_catalog.h"
#include "tachyon/transition_registry.h"
#include "tachyon/presets/transition/transition_preset_registry.h"

#include <algorithm>
#include <utility>
#include <set>

namespace tachyon {

struct TransitionCatalog::Impl {
    std::vector<TransitionCatalogEntry> entries;
    std::map<std::string, std::size_t> id_to_index;
    std::map<std::string, std::string> alias_to_id;
    std::map<std::string, std::size_t> runtime_to_index;
};

TransitionCatalog& TransitionCatalog::instance() {
    static TransitionCatalog instance;
    return instance;
}

TransitionCatalog::TransitionCatalog() {
    m_impl = std::make_unique<Impl>();
    
    // Register built-in transitions from canonical descriptors
    for (const auto& desc : get_builtin_transition_descriptors()) {
        TransitionCatalogEntry entry;
        entry.id = desc.id;
        entry.runtime_id = desc.runtime_id;
        entry.authoring_aliases = desc.authoring_aliases;
        entry.description = desc.description;
        entry.status = desc.status;
        // kind mapping if needed, or just use string
        entry.kind = "custom"; // Placeholder for more granular mapping
        
        register_entry(entry);
    }
}

TransitionCatalog::~TransitionCatalog() = default;

void TransitionCatalog::register_entry(const TransitionCatalogEntry& entry) {
    // Check for duplicate ID
    if (m_impl->id_to_index.count(entry.id)) {
        return; // Already registered
    }

    std::size_t index = m_impl->entries.size();
    m_impl->entries.push_back(entry);
    m_impl->id_to_index[entry.id] = index;
    m_impl->runtime_to_index[entry.runtime_id] = index;

    // Register aliases
    for (const auto& alias : entry.authoring_aliases) {
        m_impl->alias_to_id[alias] = entry.id;
    }
}

void TransitionCatalog::unregister_entry(std::string_view id) {
    auto it = m_impl->id_to_index.find(std::string(id));
    if (it == m_impl->id_to_index.end()) return;

    std::size_t index = it->second;
    const auto& entry = m_impl->entries[index];

    // Remove aliases
    for (const auto& alias : entry.authoring_aliases) {
        m_impl->alias_to_id.erase(alias);
    }

    // Remove from runtime map
    m_impl->runtime_to_index.erase(entry.runtime_id);

    // Remove entry
    m_impl->entries.erase(m_impl->entries.begin() + index);
    m_impl->id_to_index.erase(std::string(id));

    // Rebuild index map
    m_impl->id_to_index.clear();
    for (std::size_t i = 0; i < m_impl->entries.size(); ++i) {
        m_impl->id_to_index[m_impl->entries[i].id] = i;
    }
}

const TransitionCatalogEntry* TransitionCatalog::find(std::string_view id) const {
    auto it = m_impl->id_to_index.find(std::string(id));
    if (it == m_impl->id_to_index.end()) return nullptr;
    return &m_impl->entries[it->second];
}

const TransitionCatalogEntry* TransitionCatalog::find_by_alias(std::string_view alias) const {
    auto it = m_impl->alias_to_id.find(std::string(alias));
    if (it == m_impl->alias_to_id.end()) return nullptr;
    return find(it->second);
}

const TransitionCatalogEntry* TransitionCatalog::find_by_runtime_id(std::string_view runtime_id) const {
    auto it = m_impl->runtime_to_index.find(std::string(runtime_id));
    if (it == m_impl->runtime_to_index.end()) return nullptr;
    return &m_impl->entries[it->second];
}

std::size_t TransitionCatalog::count() const {
    return m_impl->entries.size();
}

const TransitionCatalogEntry* TransitionCatalog::get_by_index(std::size_t index) const {
    if (index >= m_impl->entries.size()) return nullptr;
    return &m_impl->entries[index];
}

std::vector<std::string> TransitionCatalog::list_all_ids() const {
    std::vector<std::string> ids;
    ids.reserve(m_impl->entries.size());
    for (const auto& entry : m_impl->entries) {
        ids.push_back(entry.id);
    }
    return ids;
}

std::vector<TransitionCatalogEntry> TransitionCatalog::list_all() const {
    return m_impl->entries;
}

bool TransitionCatalog::validate_preset_transition(std::string_view preset_id, std::string& error) const {
    // Check if it's a direct ID
    if (find(preset_id)) return true;

    // Check if it's an alias
    if (find_by_alias(preset_id)) return true;

    error = "Transition ID '" + std::string(preset_id) + "' not found in catalog";
    return false;
}

bool TransitionCatalog::validate_runtime_transition(std::string_view runtime_id, std::string& error) const {
    if (find_by_runtime_id(runtime_id)) return true;

    error = "Runtime transition ID '" + std::string(runtime_id) + "' not found in catalog";
    return false;
}

TransitionCatalog::AuditResult TransitionCatalog::audit() const {
    AuditResult result;

    auto& preset_registry = presets::TransitionPresetRegistry::instance();
    auto& transition_registry = TransitionRegistry::instance();

    // 1. Every catalog.runtime_id must exist in TransitionRegistry
    for (const auto& entry : m_impl->entries) {
        if (!entry.runtime_id.empty()) {
            const auto* runtime = transition_registry.find(entry.runtime_id);
            if (!runtime) {
                result.missing_runtime.push_back(entry.runtime_id);
            }
        }
    }

    // 2. Every public preset must exist in catalog
    auto preset_ids = preset_registry.list_ids();
    for (const auto& preset_id : preset_ids) {
        if (!find(preset_id) && !find_by_alias(preset_id)) {
            result.missing_catalog_entries.push_back(preset_id);
        }
    }

    // 3. Every public runtime transition must be cataloged
    auto runtime_ids = transition_registry.list_builtin_transition_ids();
    for (const auto& runtime_id : runtime_ids) {
        if (!find_by_runtime_id(runtime_id)) {
            result.missing_catalog_entries.push_back(runtime_id + " (runtime)");
        }
    }

    // 4. Duplicate aliases
    std::map<std::string, std::string> alias_to_id_check;
    for (const auto& entry : m_impl->entries) {
        for (const auto& alias : entry.authoring_aliases) {
            auto it = alias_to_id_check.find(alias);
            if (it != alias_to_id_check.end()) {
                result.duplicate_aliases.push_back(alias);
            } else {
                alias_to_id_check[alias] = entry.id;
            }
        }
    }

    // 5. Duplicate runtime IDs
    std::map<std::string, std::string> runtime_to_id_check;
    for (const auto& entry : m_impl->entries) {
        auto it = runtime_to_id_check.find(entry.runtime_id);
        if (it != runtime_to_id_check.end()) {
            result.duplicate_runtime_ids.push_back(entry.runtime_id);
        } else {
            runtime_to_id_check[entry.runtime_id] = entry.id;
        }
    }

    return result;
}

} // namespace tachyon
