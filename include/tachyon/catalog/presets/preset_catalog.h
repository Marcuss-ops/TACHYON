#pragma once

#include "tachyon/api.h"
#include "tachyon/catalog/presets/preset_entry.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/spec/schema/contracts/shared_contracts.h"
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <functional>

namespace tachyon::content {

/**
 * @brief Central registry for all engine presets.
 * Single source of truth for Backgrounds, Text, SFX, and Transitions.
 */
class TACHYON_API PresetCatalog {
public:
    static PresetCatalog& instance();

    /**
     * @brief Register a new preset metadata entry.
     */
    void register_entry(PresetEntry entry);

    /**
     * @brief Register a typed factory for a text animator.
     */
    void register_text_animator(std::string_view id, std::function<std::vector<tachyon::TextAnimatorSpec>(const registry::ParameterBag&)> factory);

    /**
     * @brief Register a typed factory for a background layer.
     */
    void register_background(std::string_view id, std::function<tachyon::LayerSpec(const registry::ParameterBag&)> factory);

    /**
     * @brief Register a typed factory for a sound effect.
     */
    void register_sfx(std::string_view id, std::function<tachyon::spec::AudioTrackSpec(const registry::ParameterBag&)> factory);

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
        std::string resolved_id;
    };
    ResolutionResult resolve(std::string_view id, const registry::ParameterBag& user_params) const;

    /**
     * @brief Create a Text Animator preset.
     */
    std::optional<std::vector<tachyon::TextAnimatorSpec>> create_text_animator(std::string_view id, const registry::ParameterBag& params = {}) const;

    /**
     * @brief Create a Background preset.
     */
    std::optional<tachyon::LayerSpec> create_background(std::string_view id, const registry::ParameterBag& params = {}) const;

    /**
     * @brief Create an SFX preset.
     */
    std::optional<tachyon::spec::AudioTrackSpec> create_sfx(std::string_view id, const registry::ParameterBag& params = {}) const;

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
