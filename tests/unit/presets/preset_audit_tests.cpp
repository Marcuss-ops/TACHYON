#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/studio/studio_library.h"
#include <cassert>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace tachyon {

bool run_preset_audit_tests() {
    std::cout << "Running Preset Audit tests..." << std::endl;

    auto& registry = presets::TransitionPresetRegistry::instance();
    // Use default root (current dir / studio / library)
    StudioLibrary library(""); 

    auto registered_ids = registry.list_ids();
    const auto& studio_transitions = library.transitions();
    
    // 1. Verify every studio transition asset has a matching registry entry.
    for (const auto& transition : studio_transitions) {
        if (registry.find(transition.id) == nullptr) {
            std::cerr << "Audit Error: Studio asset '" << transition.id << "' has no matching registry entry.\n";
            return false;
        }
    }

    // 2. Verify canonical presets that should be studio-visible have assets.
    // (Excluding internal/utility ones like 'none')
    for (const auto& id : registered_ids) {
        if (id == "tachyon.transition.none") continue;

        auto it = std::find_if(studio_transitions.begin(), studio_transitions.end(), 
            [&](const StudioTransitionEntry& e) { return e.id == id; });
            
        if (it == studio_transitions.end()) {
            std::cerr << "Audit Error: Registry entry '" << id << "' has no matching studio asset folder.\n";
            return false;
        }
    }

    // 3. Verify required files exist for each asset.
    for (const auto& transition : studio_transitions) {
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
        if (transition.shader_path.filename() != "v1.glsl") {
            std::cerr << "Audit Error: Asset '" << transition.id << "' shader name is not v1.glsl: " << transition.shader_path << "\n";
            return false;
        }
    }

    std::cout << "Preset Audit tests passed!" << std::endl;
    return true;
}

} // namespace tachyon
