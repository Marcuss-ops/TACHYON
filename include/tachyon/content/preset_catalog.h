#pragma once

#include "tachyon/api.h"
#include "tachyon/content/preset_entry.h"
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace tachyon::content {

/**
 * @brief Central registry for all engine presets.
 * Single source of truth for Backgrounds, Text, SFX, and Transitions.
 */
class TACHYON_API PresetCatalog {
public:
    static PresetCatalog& instance();

    /**
     * @brief Register a new preset entry.
     */
    void register_preset(PresetEntry entry);

    /**
     * @brief Find an entry by ID.
     */
    const PresetEntry* find(std::string_view id) const;

    /**
     * @brief List all IDs of a specific kind.
     */
    std::vector<std::string> list_ids(std::optional<ContentKind> kind = std::nullopt) const;

    /**
     * @brief Create an alias for an existing preset.
     * Useful for backward compatibility when merging multiple presets into one.
     */
    void register_alias(std::string alias_id, std::string target_id, registry::ParameterBag default_overrides = {});

    /**
     * @brief Resolves an ID (handling aliases) and returns the entry and merged parameters.
     */
    struct ResolutionResult {
        const PresetEntry* entry = nullptr;
        registry::ParameterBag merged_params;
    };
    ResolutionResult resolve(std::string_view id, const registry::ParameterBag& user_params) const;

    /**
     * @brief Instantiates a preset.
     */
    template <typename T>
    std::optional<T> instantiate(std::string_view id, const registry::ParameterBag& params = {}) const {
        auto res = resolve(id, params);
        if (!res.entry || !res.entry->factory) return std::nullopt;
        
        try {
            auto result = res.entry->factory(res.merged_params);
            return std::any_cast<T>(result);
        } catch (const std::bad_any_cast&) {
            return std::nullopt;
        }
    }

private:
    PresetCatalog();
    ~PresetCatalog();

    PresetCatalog(const PresetCatalog&) = delete;
    PresetCatalog& operator=(const PresetCatalog&) = delete;

    struct AliasEntry {
        std::string target_id;
        registry::ParameterBag default_overrides;
    };

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace tachyon::content
