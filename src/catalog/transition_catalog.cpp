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
    // Register built-in transitions
    register_entry({
        "tachyon.transition.fade",
        {"fade", "crossfade", "dissolve"},
        "tachyon.transition.fade",
        "fade",
        false, true,
        TransitionStatus::Stable,
        "Crossfade between layers"
    });

    register_entry({
        "tachyon.transition.slide_left",
        {"slide_left", "slide left", "slide"},
        "tachyon.transition.slide_left",
        "slide",
        true, true,
        TransitionStatus::Stable,
        "Slide layer from left"
    });

    register_entry({
        "tachyon.transition.slide_right",
        {"slide_right", "slide right"},
        "tachyon.transition.slide_right",
        "slide",
        true, true,
        TransitionStatus::Stable,
        "Slide layer from right"
    });

    register_entry({
        "tachyon.transition.zoom",
        {"zoom", "scale", "zoom_in"},
        "tachyon.transition.zoom",
        "zoom",
        false, true,
        TransitionStatus::Stable,
        "Zoom in/out transition"
    });

    register_entry({
        "tachyon.transition.flip",
        {"flip", "flip horizontal", "flip_h"},
        "tachyon.transition.flip",
        "flip",
        false, true,
        TransitionStatus::Stable,
        "Flip transition"
    });

    register_entry({
        "tachyon.transition.blur",
        {"blur", "blur transition", "blur_in"},
        "tachyon.transition.blur",
        "blur",
        false, true,
        TransitionStatus::Stable,
        "Blur transition effect"
    });

    register_entry({
        "tachyon.transition.wipe_left",
        {"wipe_left", "wipe"},
        "tachyon.transition.wipe_left",
        "wipe",
        true, true,
        TransitionStatus::Stable,
        "Wipe transition from left"
    });

    register_entry({
        "tachyon.transition.push_up",
        {"push_up", "push", "slide_up"},
        "tachyon.transition.push_up",
        "push",
        true, true,
        TransitionStatus::Stable,
        "Push layer upward"
    });

    register_entry({
        "tachyon.transition.glitch",
        {"glitch", "digital", "glitch_effect"},
        "tachyon.transition.glitch",
        "glitch",
        false, true,
        TransitionStatus::Experimental,
        "Digital glitch transition effect"
    });

    register_entry({
        "tachyon.transition.cinematic_fast",
        {"cinematic_fast", "fast_cut", "quick_cut"},
        "tachyon.transition.cinematic_fast",
        "cut",
        false, false,
        TransitionStatus::Stable,
        "Fast cinematic cut (no duration)"
    });
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

    // Check 1: Every catalog.runtime_id must exist in TransitionRegistry
    for (const auto& entry : m_impl->entries) {
        if (!entry.runtime_id.empty()) {
            const auto* runtime = transition_registry.find(entry.runtime_id);
            if (!runtime) {
                result.orphaned_runtime.push_back("Catalog entry '" + entry.id +
                    "' has runtime_id '" + entry.runtime_id + "' not found in TransitionRegistry");
            }
        }
    }

    // Check 2: Every public preset must exist in catalog
    auto preset_ids = preset_registry.list_ids();
    for (const auto& preset_id : preset_ids) {
        if (!find(preset_id)) {
            result.orphaned_presets.push_back("Preset '" + preset_id + "' not found in catalog");
        }
    }

    // Check 3: Every public runtime transition must be cataloged
    auto runtime_ids = transition_registry.list_builtin_transition_ids();
    for (const auto& runtime_id : runtime_ids) {
        const auto* catalog_entry = find_by_runtime_id(runtime_id);
        if (!catalog_entry) {
            result.orphaned_runtime.push_back("Runtime transition '" + runtime_id + "' not cataloged");
        }
    }

    // Check 4: Duplicate aliases
    std::map<std::string, std::string> alias_to_id_check;
    for (const auto& entry : m_impl->entries) {
        for (const auto& alias : entry.authoring_aliases) {
            auto it = alias_to_id_check.find(alias);
            if (it != alias_to_id_check.end()) {
                result.alias_conflicts.push_back("Alias '" + alias + "' points to both '" +
                    it->second + "' and '" + entry.id + "'");
            } else {
                alias_to_id_check[alias] = entry.id;
            }
        }
    }

    // Check 5: Duplicate catalog IDs (should not happen due to registration logic, but verify)
    std::set<std::string> seen_ids;
    for (const auto& entry : m_impl->entries) {
        if (seen_ids.count(entry.id)) {
            result.alias_conflicts.push_back("Duplicate catalog ID: '" + entry.id + "'");
        }
        seen_ids.insert(entry.id);
    }

    // Check 6: amber_sweep must be registered everywhere (if it exists)
    const std::string amber_sweep_id = "tachyon.transition.lightleak.amber_sweep";
    const auto* amber_catalog = find(amber_sweep_id);
    const auto* amber_preset = preset_registry.find(amber_sweep_id);
    const auto* amber_runtime = transition_registry.find(amber_sweep_id);

    if (amber_catalog || amber_preset || amber_runtime) {
        // If registered in any, should be in all
        if (!amber_catalog) {
            result.orphaned_presets.push_back(amber_sweep_id + " found in registry/preset but not in catalog");
        }
        if (!amber_preset) {
            result.orphaned_presets.push_back(amber_sweep_id + " found in catalog/registry but not in preset registry");
        }
        if (!amber_runtime) {
            result.orphaned_runtime.push_back(amber_sweep_id + " found in catalog/preset but not in runtime registry");
        }
    }

    return result;
}

} // namespace tachyon
