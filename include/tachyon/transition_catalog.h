#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <memory>
#include <map>
#include "tachyon/core/spec/schema/common/common_spec.h"

namespace tachyon {

enum class TransitionStatus {
    Stable,
    Experimental,
    Deprecated
};

struct CatalogTransitionDescriptor {
    std::string id;
    std::string runtime_id;
    std::string name;
    std::string description;
    TransitionKind kind{TransitionKind::None};
    TransitionStatus status{TransitionStatus::Stable};
    std::vector<std::string> authoring_aliases;
    bool visible_in_catalog{true};
    bool requires_runtime_function{true};
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

std::vector<CatalogTransitionDescriptor> get_builtin_transition_descriptors();

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
        std::vector<std::string> missing_runtime;
        std::vector<std::string> missing_catalog_entries;
        std::vector<std::string> duplicate_aliases;
        std::vector<std::string> duplicate_runtime_ids;
        [[nodiscard]] bool ok() const {
            return missing_runtime.empty() && 
                   missing_catalog_entries.empty() && 
                   duplicate_aliases.empty() && 
                   duplicate_runtime_ids.empty();
        }
    };
    AuditResult audit() const;

private:
    TransitionCatalog();
    ~TransitionCatalog();
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tachyon
