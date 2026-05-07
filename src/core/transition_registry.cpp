#include "tachyon/transition_registry.h"

#include <algorithm>
#include <utility>
#include <iostream>

namespace tachyon {

struct TransitionRegistry::Impl {
    std::unordered_map<std::string, TransitionDescriptor> descriptors; // id -> descriptor
    std::unordered_map<std::string, std::string> alias_to_id;         // alias -> id
};

TransitionRegistry::TransitionRegistry() : m_impl(std::make_unique<Impl>()) {}
TransitionRegistry::~TransitionRegistry() = default;

void TransitionRegistry::register_descriptor(const TransitionDescriptor& descriptor) {
    if (descriptor.id.empty()) {
        return;
    }
    
    // Check for duplicates - always warn and keep first registration
    auto it = m_impl->descriptors.find(descriptor.id);
    if (it != m_impl->descriptors.end()) {
        std::cerr << "WARNING: Duplicate transition '" << descriptor.id << "' ignored\n";
        return;
    }
    
    // Register main descriptor
    m_impl->descriptors[descriptor.id] = descriptor;
    // Register aliases
    for (const auto& alias : descriptor.aliases) {
        if (!alias.empty()) {
            m_impl->alias_to_id[alias] = descriptor.id;
        }
    }
}

const TransitionDescriptor* TransitionRegistry::find_by_id(std::string_view id) const {
    auto it = m_impl->descriptors.find(std::string(id));
    return it != m_impl->descriptors.end() ? &it->second : nullptr;
}

const TransitionDescriptor* TransitionRegistry::find_by_alias(std::string_view alias) const {
    auto alias_it = m_impl->alias_to_id.find(std::string(alias));
    if (alias_it != m_impl->alias_to_id.end()) {
        return find_by_id(alias_it->second);
    }
    return nullptr;
}

const TransitionDescriptor* TransitionRegistry::resolve(std::string_view id_or_alias) const {
    auto* by_id = find_by_id(id_or_alias);
    return by_id ? by_id : find_by_alias(id_or_alias);
}

std::vector<TransitionCatalogEntry> TransitionRegistry::catalog_entries() const {
    std::vector<TransitionCatalogEntry> entries;
    entries.reserve(m_impl->descriptors.size());
    for (const auto& [id, desc] : m_impl->descriptors) {
        entries.push_back({
            desc.id,
            desc.display_name,
            desc.description,
            desc.category,
            desc.aliases,
            desc.capabilities.supports_cpu,
            desc.capabilities.supports_gpu
        });
    }
    return entries;
}

std::vector<std::string> TransitionRegistry::list_all_ids() const {
    std::vector<std::string> ids;
    ids.reserve(m_impl->descriptors.size());
    for (const auto& [id, _] : m_impl->descriptors) {
        ids.push_back(id);
    }
    return ids;
}

std::vector<const TransitionDescriptor*> TransitionRegistry::list_all() const {
    std::vector<const TransitionDescriptor*> descs;
    descs.reserve(m_impl->descriptors.size());
    for (const auto& [id, desc] : m_impl->descriptors) {
        descs.push_back(&desc);
    }
    return descs;
}

void TransitionRegistry::unregister_transition(std::string_view id) {
    auto it = m_impl->descriptors.find(std::string(id));
    if (it != m_impl->descriptors.end()) {
        // Remove associated aliases
        for (const auto& alias : it->second.aliases) {
            m_impl->alias_to_id.erase(alias);
        }
        m_impl->descriptors.erase(it);
    }
}

}  // namespace tachyon
