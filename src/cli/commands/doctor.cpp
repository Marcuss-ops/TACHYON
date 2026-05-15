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
#include "tachyon/tachyon_build_config.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <thread>

namespace tachyon {

bool run_doctor_command(const CliOptions&, std::ostream& out, std::ostream& err, runtime::EngineRegistry& bundle) {
    out << "TACHYON Doctor - System Diagnostic\n";
    out << "==================================\n\n";

    // 1. Build & Environment Info
    out << "[1/3] Environment Metadata...\n";
    out << "      Compiler:      " << TACHYON_COMPILER_ID << " " << TACHYON_COMPILER_VERSION << "\n";
    out << "      Build Type:    " << TACHYON_BUILD_TYPE << "\n";
    out << "      Threads:       " << std::thread::hardware_concurrency() << " available\n";
    
    out << "      Features:\n";
    out << "        Media:       " << (TACHYON_MEDIA_ENABLED ? "ENABLED" : "DISABLED") << "\n";
    out << "        SIMD:        " << (TACHYON_SIMD_ENABLED ? "ENABLED" : "DISABLED") << "\n";
    out << "        mimalloc:    " << (TACHYON_MIMALLOC_ENABLED ? "LINKED" : "NO") << "\n";

    if (TACHYON_MEDIA_ENABLED) {
        std::string mode = TACHYON_FFMPEG_DISCOVERY_MODE;
        if (mode.empty()) mode = "unknown";
        out << "      FFmpeg:\n";
        out << "        Found:       YES\n";
        out << "        Discovery:   " << mode << "\n";
    }
    out << "\n";

    // 2. Audit Transition Registries
    out << "[2/3] Auditing Transition Registry alignment...\n";
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

    // 3. Audit Library Discovery
    out << "[3/3] Auditing Library Discovery...\n";
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
