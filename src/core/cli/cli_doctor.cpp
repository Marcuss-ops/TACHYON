#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/catalog/catalog.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "cli_internal.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

namespace tachyon {

bool run_doctor_command(const CliOptions&, std::ostream& out, std::ostream& err) {
    out << "TACHYON Doctor - System Diagnostic\n";
    out << "==================================\n\n";

    // 1. Audit Transition Registries
    out << "[1/2] Auditing Transition Registry alignment...\n";
    const auto& preset_reg = presets::TransitionPresetRegistry::instance();
    renderer2d::init_builtin_transitions();
    const auto& runtime_reg = TransitionRegistry::instance();
    
    auto preset_ids = preset_reg.list_ids();
    int preset_errors = 0;
    for (const auto& id : preset_ids) {
        if (id == "none" || id == "tachyon.transition.none") continue;
        
        auto spec = preset_reg.create(id, {});
        if (spec.transition_id.empty()) {
            err << "      [ERROR] Preset '" << id << "' produced empty transition_id\n";
            preset_errors++;
            continue;
        }
        
        const auto* runtime_spec = runtime_reg.find(spec.transition_id);
        if (runtime_spec == nullptr) {
            err << "      [ERROR] Preset '" << id << "' resolves to unknown runtime ID: " << spec.transition_id << "\n";
            preset_errors++;
            continue;
        }

        if (runtime_spec->function == nullptr
            && runtime_spec->state_type == TransitionSpec::Type::None) {
            err << "      [ERROR] Runtime transition '" << spec.transition_id
                << "' has neither a pixel function nor a state type\n";
            preset_errors++;
        }
    }
    
    if (preset_errors == 0) {
        out << "      Success: All " << preset_ids.size() << " presets are aligned with runtime functions.\n";
    } else {
        out << "      Failed: " << preset_errors << " alignment errors found.\n";
    }
    out << "\n";

    // 2. Audit Catalog Discovery
    out << "[2/2] Auditing Catalog Discovery...\n";
    // We provide a default root if needed, but the catalog constructor handles it.
    TachyonCatalog catalog(""); 
    int catalog_errors = 0;
    
    for (const auto& id : preset_ids) {
        if (id == "none" || id == "tachyon.transition.none") continue;
        if (!catalog.find_transition(id).has_value()) {
            err << "      [ERROR] Transition '" << id << "' not found in catalog listing.\n";
            catalog_errors++;
        }
    }
    
    if (catalog_errors == 0) {
        out << "      Success: All registered transitions are discoverable in the catalog.\n";
    } else {
        out << "      Failed: " << catalog_errors << " transitions missing from catalog.\n";
    }
    out << "\n";

    bool ok = (preset_errors == 0 && catalog_errors == 0);
    if (ok) {
        out << "Overall Status: HEALTHY\n";
    } else {
        out << "Overall Status: ISSUES DETECTED\n";
    }

    return ok;
}

} // namespace tachyon
