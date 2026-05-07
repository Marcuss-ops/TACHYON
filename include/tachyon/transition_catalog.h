#pragma once

#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/transition_registry.h"
#include <vector>
#include <string>
#include <string_view>

namespace tachyon {

class TransitionCatalog {
public:
    // Returns catalog entries derived from the canonical TransitionRegistry.
    // The catalog no longer stores its own entries; it reflects the registry state.
    static std::vector<TransitionCatalogEntry> entries() {
        return TransitionRegistry::instance().catalog_entries();
    }

    // Backward-compatible helpers
    static std::vector<std::string> list_all_ids() {
        return TransitionRegistry::instance().list_all_ids();
    }

    static const TransitionDescriptor* find(std::string_view id) {
        return TransitionRegistry::instance().resolve(id);
    }

    // Validation: check if a preset/runtime transition exists in the registry
    static bool validate_preset_transition(std::string_view preset_id, std::string& error) {
        const auto* desc = TransitionRegistry::instance().resolve(preset_id);
        if (!desc) {
            error = "Preset transition '" + std::string(preset_id) + "' not found in registry.";
            return false;
        }
        return true;
    }

    static bool validate_runtime_transition(std::string_view runtime_id, std::string& error) {
        return validate_preset_transition(runtime_id, error);
    }
};

} // namespace tachyon
