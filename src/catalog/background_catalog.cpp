#include "tachyon/background_catalog.h"
#include "tachyon/background_descriptor.h"
#include "tachyon/backgrounds.hpp"

#include <algorithm>
#include <string>
#include <set>

namespace tachyon {

namespace {

BackgroundCatalogEntry to_catalog_entry(const BackgroundDescriptor& desc) {
    BackgroundCatalogEntry entry;
    entry.id = desc.id;

    switch (desc.kind) {
        case BackgroundKind::Solid:
            entry.role = BackgroundCatalogRole::Solid;
            break;
        case BackgroundKind::LinearGradient:
        case BackgroundKind::RadialGradient:
            entry.role = BackgroundCatalogRole::Gradient;
            break;
        case BackgroundKind::Image:
            entry.role = BackgroundCatalogRole::Image;
            break;
        case BackgroundKind::Video:
        case BackgroundKind::Procedural:
            entry.role = BackgroundCatalogRole::Procedural;
            break;
    }

    entry.preset_params = "{}";
    entry.procedural_factory_id = (desc.kind == BackgroundKind::Procedural) ? desc.id : "";
    entry.status = desc.status;
    entry.description = desc.description;
    return entry;
}

std::vector<BackgroundCatalogEntry> to_catalog_entries(const std::vector<const BackgroundDescriptor*>& descriptors) {
    std::vector<BackgroundCatalogEntry> entries;
    entries.reserve(descriptors.size());
    for (const auto* desc : descriptors) {
        if (desc) {
            entries.push_back(to_catalog_entry(*desc));
        }
    }
    return entries;
}

} // namespace

BackgroundCatalog::BackgroundCatalog(BackgroundRegistry& registry) : registry_(registry) {
}

void BackgroundCatalog::register_entry(const BackgroundCatalogEntry& entry) {
    // BackgroundCatalog is deprecated. Use BackgroundRegistry::register_descriptor() directly.
    // This method performs a lossy conversion for legacy compatibility only.
    auto* existing = registry_.resolve(entry.id);
    if (existing) return; // Already registered

    // Convert to BackgroundDescriptor (lossy - BackgroundCatalogEntry has fields
    // like status, preset_params that BackgroundDescriptor doesn't have)
    BackgroundDescriptor desc;
    desc.id = entry.id;
    desc.description = entry.description;

    // Map role to kind
    switch (entry.role) {
        case BackgroundCatalogRole::Solid: desc.kind = BackgroundKind::Solid; break;
        case BackgroundCatalogRole::Gradient: desc.kind = BackgroundKind::LinearGradient; break;
        case BackgroundCatalogRole::Image: desc.kind = BackgroundKind::Image; break;
        case BackgroundCatalogRole::Procedural: desc.kind = BackgroundKind::Procedural; break;
    }

    registry_.register_descriptor(desc);
}

void BackgroundCatalog::unregister_entry(std::string_view id) {
    registry_.unregister_background(id);
}

const BackgroundCatalogEntry* BackgroundCatalog::find(std::string_view id) const {
    auto* desc = registry_.resolve(id);
    if (!desc) return nullptr;

    static BackgroundCatalogEntry temp;
    temp = to_catalog_entry(*desc);
    return &temp;
}

std::size_t BackgroundCatalog::count() const {
    return registry_.list_all_ids().size();
}

const BackgroundCatalogEntry* BackgroundCatalog::get_by_index(std::size_t index) const {
    static std::vector<BackgroundCatalogEntry> cache;
    cache = to_catalog_entries(registry_.list_all());
    if (index >= cache.size()) return nullptr;
    return &cache[index];
}

std::vector<std::string> BackgroundCatalog::list_all_ids() const {
    return registry_.list_all_ids();
}

std::vector<BackgroundCatalogEntry> BackgroundCatalog::list_all() const {
    return to_catalog_entries(registry_.list_all());
}

bool BackgroundCatalog::validate_preset(std::string_view preset_id, std::string& error) const {
    if (registry_.resolve(preset_id)) return true;

    error = "Background preset ID '" + std::string(preset_id) + "' not found in registry";
    return false;
}

BackgroundCatalog::AuditResult BackgroundCatalog::audit() const {
    AuditResult result;
    auto descriptors = registry_.list_all();

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
