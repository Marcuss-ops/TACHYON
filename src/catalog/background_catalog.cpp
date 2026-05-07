#include "tachyon/background_catalog.h"
#include "tachyon/background_descriptor.h"
#include "tachyon/backgrounds.hpp"

#include <algorithm>
#include <string>
#include <set>

namespace tachyon {

// Helper to convert BackgroundDescriptor to BackgroundCatalogEntry
static BackgroundCatalogEntry descriptor_to_catalog_entry(const BackgroundDescriptor& desc) {
    BackgroundCatalogEntry entry;
    entry.id = desc.id;

    // Map kind to role
    switch (desc.kind) {
        case BackgroundKind::Solid: entry.role = BackgroundCatalogRole::Solid; break;
        case BackgroundKind::LinearGradient:
        case BackgroundKind::RadialGradient: entry.role = BackgroundCatalogRole::Gradient; break;
        case BackgroundKind::Image: entry.role = BackgroundCatalogRole::Image; break;
        case BackgroundKind::Video:
        case BackgroundKind::Procedural: entry.role = BackgroundCatalogRole::Procedural; break;
    }

    // TODO: serialize desc.params to JSON string for preset_params
    entry.preset_params = "{}";
    entry.procedural_factory_id = (desc.kind == BackgroundKind::Procedural) ? desc.id : "";
    entry.status = desc.status; // Use status from descriptor
    entry.description = desc.description;

    return entry;
}

BackgroundCatalog& BackgroundCatalog::instance() {
    static BackgroundCatalog instance;
    return instance;
}

BackgroundCatalog::BackgroundCatalog() {
    // Built-in backgrounds are now registered in BackgroundRegistry constructor
}

BackgroundCatalog::~BackgroundCatalog() = default;

void BackgroundCatalog::register_entry(const BackgroundCatalogEntry& entry) {
    // BackgroundCatalog is deprecated. Use BackgroundRegistry::register_descriptor() directly.
    auto* existing = BackgroundRegistry::instance().resolve(entry.id);
    if (existing) return; // Already registered

    // Convert to BackgroundDescriptor
    BackgroundDescriptor desc;
    desc.id = entry.id;
    desc.description = entry.description;
    desc.status = static_cast<BackgroundStatus>(entry.status); // Cast legacy status

    // Map role to kind
    switch (entry.role) {
        case BackgroundCatalogRole::Solid: desc.kind = BackgroundKind::Solid; break;
        case BackgroundCatalogRole::Gradient: desc.kind = BackgroundKind::LinearGradient; break;
        case BackgroundCatalogRole::Image: desc.kind = BackgroundKind::Image; break;
        case BackgroundCatalogRole::Procedural: desc.kind = BackgroundKind::Procedural; break;
    }

    BackgroundRegistry::instance().register_descriptor(desc);
}

void BackgroundCatalog::unregister_entry(std::string_view id) {
    BackgroundRegistry::instance().unregister_background(id);
}

const BackgroundCatalogEntry* BackgroundCatalog::find(std::string_view id) const {
    auto* desc = BackgroundRegistry::instance().resolve(id);
    if (!desc) return nullptr;

    // Return a static copy (thin wrapper limitation)
    static BackgroundCatalogEntry temp;
    temp = descriptor_to_catalog_entry(*desc);
    return &temp;
}

std::size_t BackgroundCatalog::count() const {
    return BackgroundRegistry::instance().list_all_ids().size();
}

const BackgroundCatalogEntry* BackgroundCatalog::get_by_index(std::size_t index) const {
    auto descriptors = BackgroundRegistry::instance().list_all();
    if (index >= descriptors.size()) return nullptr;

    // Convert descriptors to catalog entries
    static std::vector<BackgroundCatalogEntry> cache;
    cache.clear();
    cache.reserve(descriptors.size());
    for (const auto* desc : descriptors) {
        cache.push_back(descriptor_to_catalog_entry(*desc));
    }

    return &cache[index];
}

std::vector<std::string> BackgroundCatalog::list_all_ids() const {
    return BackgroundRegistry::instance().list_all_ids();
}

std::vector<BackgroundCatalogEntry> BackgroundCatalog::list_all() const {
    auto descriptors = BackgroundRegistry::instance().list_all();
    std::vector<BackgroundCatalogEntry> entries;
    entries.reserve(descriptors.size());
    for (const auto* desc : descriptors) {
        entries.push_back(descriptor_to_catalog_entry(*desc));
    }
    return entries;
}

bool BackgroundCatalog::validate_preset(std::string_view preset_id, std::string& error) const {
    if (BackgroundRegistry::instance().resolve(preset_id)) return true;

    error = "Background preset ID '" + std::string(preset_id) + "' not found in registry";
    return false;
}

BackgroundCatalog::AuditResult BackgroundCatalog::audit() const {
    AuditResult result;
    auto descriptors = BackgroundRegistry::instance().list_all();

    // Check for duplicates
    std::set<std::string> seen_ids;
    for (const auto* desc : descriptors) {
        if (seen_ids.count(desc->id)) {
            result.duplicate_ids.push_back(desc->id);
        }
        seen_ids.insert(desc->id);
    }

    return result;
}

} // namespace tachyon
