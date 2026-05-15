#include "tachyon/runtime/registry/engine_registry.h"
#include "tachyon/core/cli.h"
#include "tachyon/core/cli_options.h"
#include "tachyon/core/library/library.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "cli/support/cli_internal.h"
#include "command_registry.h"
#include <iostream>
#include "tachyon/runtime/registry/engine_registry.h"
#include <iomanip>
#include <algorithm>

namespace tachyon {

bool run_doctor_command(const CliOptions&, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle) {
    out << "TACHYON Doctor - System Diagnostic\n";
    out << "==================================\n\n";

    // Audit Transition Registries
    out << "[1/2] Auditing Transition Registry alignment...\n";
    presets::TransitionPresetRegistry preset_reg;

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
        
        const auto* desc = bundle.transitions.resolve(spec.transition_id);
        if (desc == nullptr) {
            err << "      [ERROR] Preset '" << id << "' resolves to unknown runtime ID: " << spec.transition_id << "\n";
            preset_errors++;
            continue;
        }

        if (desc->cpu_fn == nullptr 
            && desc->glsl_fn == nullptr 
            && desc->runtime_kind == TransitionRuntimeKind::StateOnly) {
            err << "      [ERROR] Runtime transition '" << spec.transition_id
                << "' has neither CPU nor GLSL function and is not a state-only transition\n";
            preset_errors++;
        }
    }
    
    if (preset_errors == 0) {
        out << "      Success: All " << preset_ids.size() << " presets are aligned with runtime functions.\n";
    } else {
        out << "      Failed: " << preset_errors << " alignment errors found.\n";
    }
    out << "\n";

    // 2. Audit Library Discovery
    out << "[2/2] Auditing Library Discovery...\n";
    // We provide a default root if needed, but the library constructor handles it.
    TachyonLibrary library(""); 
    int library_errors = 0;
    
    for (const auto& id : preset_ids) {
        if (id == "none" || id == "tachyon.transition.none") continue;
        if (!library.find_transition(id).has_value()) {
            err << "      [ERROR] Transition '" << id << "' not found in library listing.\n";
            library_errors++;
        }
    }
    
    if (library_errors == 0) {
        out << "      Success: All registered transitions are discoverable in the library.\n";
    } else {
        out << "      Failed: " << library_errors << " transitions missing from library.\n";
    }
    out << "\n";

    bool ok = (preset_errors == 0 && library_errors == 0);
    if (ok) {
        out << "Overall Status: HEALTHY\n";
    } else {
        out << "Overall Status: ISSUES DETECTED\n";
    }

    return ok;
}

CommandDescriptor make_doctor_command() {
    return {
        "doctor",
        "tachyon doctor",
        nullptr,
        run_doctor_command
    };
}

} // namespace tachyon
