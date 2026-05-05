#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/core/catalog/catalog.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace tachyon {

bool run_preset_audit_tests() {
    std::cout << "Running Preset Audit tests..." << std::endl;

    auto& registry = presets::TransitionPresetRegistry::instance();
    // Use default root (current dir / assets / catalog)
    TachyonCatalog catalog(""); 

    auto registered_ids = registry.list_ids();
    const auto& catalog_transitions = catalog.transitions();
    
    if (catalog_transitions.empty()) {
        std::cout << "Skipping Preset Audit: Catalog transitions are empty (no assets found at " << catalog.root().generic_string() << ").\n";
        return true;
    }

    // 1. Verify every catalog transition asset has a matching registry entry.
    for (const auto& transition : catalog_transitions) {
        if (registry.find(transition.id) == nullptr) {
            std::cerr << "Audit Error: Catalog asset '" << transition.id << "' has no matching registry entry.\n";
            return false;
        }
    }

    // 2. Verify canonical presets that should be catalog-visible have assets.
    // (Excluding internal/utility ones like 'none')
    for (const auto& id : registered_ids) {
        if (id == "tachyon.transition.none") continue;

        auto it = std::find_if(catalog_transitions.begin(), catalog_transitions.end(), 
            [&](const CatalogTransitionEntry& e) { return e.id == id; });
            
        if (it == catalog_transitions.end()) {
            std::cerr << "Audit Error: Registry entry '" << id << "' has no matching catalog asset folder.\n";
            return false;
        }
    }

    // 3. Verify required files exist for each asset.
    for (const auto& transition : catalog_transitions) {
        if (!std::filesystem::exists(transition.shader_path)) {
            std::cerr << "Audit Error: Asset '" << transition.id << "' missing shader: " << transition.shader_path << "\n";
            return false;
        }
        
        // According to USER_REQUEST, thumb.png is required.
        if (!std::filesystem::exists(transition.thumb_path)) {
             std::cerr << "Audit Error: Asset '" << transition.id << "' missing thumb: " << transition.thumb_path << "\n";
             return false; 
        }

        // Verify path conventions
        if (transition.shader_path.extension() != ".glsl") {
            std::cerr << "Audit Error: Asset '" << transition.id << "' shader extension is not .glsl: " << transition.shader_path << "\n";
            return false;
        }

        if (transition.shader_path.parent_path() != catalog.root() / "transitions") {
            std::cerr << "Audit Error: Asset '" << transition.id << "' shader path is not flat: " << transition.shader_path << "\n";
            return false;
        }
    }

    std::cout << "Preset Audit tests passed!" << std::endl;
    return true;
}

} // namespace tachyon
