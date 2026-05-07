#include "tachyon/background_catalog.h"
#include "tachyon/background_descriptor.h"
#include "tachyon/backgrounds.hpp"

#include <algorithm>
#include <string>
#include <set>

namespace tachyon {

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
    // This method performs a lossy conversion for legacy compatibility only.
    auto* existing = BackgroundRegistry::instance().resolve(entry.id);
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
    temp.id = desc->id;

    // Map kind to role
    switch (desc->kind) {
        case BackgroundKind::Solid: temp.role = BackgroundCatalogRole::Solid; break;
        case BackgroundKind::LinearGradient:
        case BackgroundKind::RadialGradient: temp.role = BackgroundCatalogRole::Gradient; break;
        case BackgroundKind::Image: temp.role = BackgroundCatalogRole::Image; break;
        case BackgroundKind::Video:
        case BackgroundKind::Procedural: temp.role = BackgroundCatalogRole::Procedural; break;
    }

    temp.preset_params = "{}"; // TODO: serialize desc.params
    temp.procedural_factory_id = (desc->kind == BackgroundKind::Procedural) ? desc->id : "";
    temp.status = BackgroundStatus::Stable; // BackgroundDescriptor doesn't carry status
    temp.description = desc->description;

    return &temp;
}

std::size_t BackgroundCatalog::count() const {
    return BackgroundRegistry::instance().list_all_ids().size();
}

const BackgroundCatalogEntry* BackgroundCatalog::get_by_index(std::size_t index) const {
    auto entries = BackgroundRegistry::instance().catalog_entries();
    if (index >= entries.size()) return nullptr;
    // Return address of vector element (valid until next call)
    static std::vector<BackgroundCatalogEntry> cache;
    cache = std::move(entries);
    return &cache[index];
}

std::vector<std::string> BackgroundCatalog::list_all_ids() const {
    return BackgroundRegistry::instance().list_all_ids();
}

std::vector<BackgroundCatalogEntry> BackgroundCatalog::list_all() const {
    return BackgroundRegistry::instance().catalog_entries();
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
