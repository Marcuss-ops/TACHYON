#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <memory>
#include <map>

namespace tachyon {

enum class BackgroundStatus {
    Stable,
    Experimental,
    Deprecated
};

enum class BackgroundCatalogRole {
    Solid,
    Gradient,
    Procedural,
    Image
};

struct BackgroundCatalogEntry {
    std::string id;
    BackgroundCatalogRole role{BackgroundCatalogRole::Solid};
    std::string preset_params; // JSON schema for expected params
    std::string procedural_factory_id;
    BackgroundStatus status{BackgroundStatus::Stable};
    std::string description;
};

class BackgroundCatalog {
public:
    static BackgroundCatalog& instance();

    void register_entry(const BackgroundCatalogEntry& entry);
    void unregister_entry(std::string_view id);

    const BackgroundCatalogEntry* find(std::string_view id) const;

    std::size_t count() const;
    const BackgroundCatalogEntry* get_by_index(std::size_t index) const;

    std::vector<std::string> list_all_ids() const;
    std::vector<BackgroundCatalogEntry> list_all() const;

    // Validation
    bool validate_preset(std::string_view preset_id, std::string& error) const;

    // Audit
    struct AuditResult {
        std::vector<std::string> missing_catalog_entries;
        std::vector<std::string> missing_factories;
        std::vector<std::string> duplicate_ids;
        [[nodiscard]] bool ok() const {
            return missing_catalog_entries.empty() && 
                   missing_factories.empty() && 
                   duplicate_ids.empty();
        }
    };
    AuditResult audit() const;

private:
    BackgroundCatalog();
    ~BackgroundCatalog();
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tachyon
