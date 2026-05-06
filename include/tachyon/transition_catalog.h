#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <memory>
#include <map>

namespace tachyon {

enum class TransitionStatus {
    Stable,
    Experimental,
    Deprecated
};

struct TransitionCatalogEntry {
    std::string id;
    std::vector<std::string> authoring_aliases;
    std::string runtime_id;
    std::string kind; // "fade", "slide", "zoom", "flip", "blur", etc.
    bool supports_direction{false};
    bool supports_duration{true};
    TransitionStatus status{TransitionStatus::Stable};
    std::string description;
};

class TransitionCatalog {
public:
    static TransitionCatalog& instance();

    void register_entry(const TransitionCatalogEntry& entry);
    void unregister_entry(std::string_view id);

    const TransitionCatalogEntry* find(std::string_view id) const;
    const TransitionCatalogEntry* find_by_alias(std::string_view alias) const;
    const TransitionCatalogEntry* find_by_runtime_id(std::string_view runtime_id) const;

    std::size_t count() const;
    const TransitionCatalogEntry* get_by_index(std::size_t index) const;

    std::vector<std::string> list_all_ids() const;
    std::vector<TransitionCatalogEntry> list_all() const;

    // Validation
    bool validate_preset_transition(std::string_view preset_id, std::string& error) const;
    bool validate_runtime_transition(std::string_view runtime_id, std::string& error) const;

    // Audit
    struct AuditResult {
        std::vector<std::string> orphaned_presets;     // Presets without runtime
        std::vector<std::string> orphaned_runtime;     // Runtime without catalog entry
        std::vector<std::string> deprecated_in_use;     // Deprecated but still referenced
        std::vector<std::string> alias_conflicts;       // Aliases pointing to non-existent IDs
    };
    AuditResult audit() const;

private:
    TransitionCatalog();
    ~TransitionCatalog();
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tachyon
