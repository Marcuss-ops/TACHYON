#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/transition_registry.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
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

    // 4. Runtime Registry Alignment Audit
    renderer2d::init_builtin_transitions(); // Ensure built-ins are loaded
    auto& runtime_reg = TransitionRegistry::instance();

    for (const auto& id : registered_ids) {
        if (id == "tachyon.transition.none") continue;

        // 4a. Check if it creates a valid spec
        registry::ParameterBag empty_bag;
        LayerTransitionSpec spec = registry.create(id, empty_bag);

        if (spec.transition_id.empty()) {
            std::cerr << "Audit Error: Preset '" << id << "' created a spec with empty transition_id.\n";
            return false;
        }

        // 4b. Check if transition_id exists in runtime registry
        const TransitionSpec* runtime_spec = runtime_reg.find(spec.transition_id);
        if (runtime_spec == nullptr) {
            std::cerr << "Audit Error: Preset '" << id << "' maps to runtime ID '" << spec.transition_id << "' which is MISSING from TransitionRegistry.\n";
            return false;
        }

        // 4c. Check if it has a pixel function OR a valid state type
        if (runtime_spec->function == nullptr && runtime_spec->state_type == TransitionSpec::Type::None) {
            std::cerr << "Audit Error: Runtime transition '" << spec.transition_id << "' has neither a pixel function nor a state type.\n";
            return false;
        }
    }

    std::cout << "Preset Audit tests passed!" << std::endl;
    return true;
}

} // namespace tachyon
