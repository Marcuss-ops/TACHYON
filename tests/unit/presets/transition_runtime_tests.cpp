#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <cassert>
#include <iostream>
#include <set>

bool run_transition_runtime_tests() {
    using namespace tachyon;
    
    std::cout << "Running Transition Runtime tests..." << std::endl;
    
    // Create local registry and load built-ins
    TransitionRegistry registry;
    register_builtin_transitions(registry);
    renderer2d::init_builtin_transitions(registry);
    auto& runtime = registry;
 
    // 1. Registry must not be empty after init.
    assert(runtime.list_all_ids().size() > 0);
 
    // 2. Every registered runtime transition must have either a pixel function
    //    or a valid state type — no orphan entries.
    const auto runtime_ids = runtime.list_all_ids();
    for (const auto& id : runtime_ids) {
        const TransitionDescriptor* desc = runtime.find_by_id(id);
        if (desc == nullptr) {
            std::cerr << "TransitionRuntime: id '" << id << "' listed but not findable.\n";
            return false;
        }
 
        const bool has_impl =
            desc->cpu_fn != nullptr ||
            desc->glsl_fn != nullptr ||
            desc->runtime_kind == TransitionRuntimeKind::StateOnly;
 
        if (!has_impl) {
            std::cerr << "TransitionRuntime: '" << id << "' has no pixel function, or state type.\n";
            return false;
        }
    }
 
    // 3. Reverse audit: every preset that maps to a runtime id must resolve.
    auto& preset_registry = presets::TransitionPresetRegistry::instance();
    const auto preset_ids = preset_registry.list_ids();
 
    for (const auto& pid : preset_ids) {
        if (pid == "tachyon.transition.none") continue;
 
        LayerTransitionSpec spec = preset_registry.create(pid, {});
 
        if (spec.transition_id.empty()) {
            std::cerr << "TransitionRuntime: preset '" << pid << "' produced empty transition_id.\n";
            return false;
        }
 
        const TransitionDescriptor* runtime_desc = runtime.resolve(spec.transition_id);
        if (runtime_desc == nullptr) {
            std::cerr << "TransitionRuntime: preset '" << pid << "' → runtime id '"
                      << spec.transition_id << "' NOT FOUND. Registry drift detected.\n";
            return false;
        }
    }
 
    // 4. No duplicate runtime ids.
    {
        std::set<std::string> seen;
        for (const auto& id : runtime_ids) {
            if (!seen.insert(id).second) {
                std::cerr << "TransitionRuntime: duplicate runtime id '" << id << "'.\n";
                return false;
            }
        }
    }
 
    // 5. Specific key transitions must always be present.
    static constexpr std::string_view kRequired[] = {
        tachyon::ids::transition::crossfade,
        tachyon::ids::transition::luma_dissolve,
        tachyon::ids::transition::slide_up,
        tachyon::ids::transition::swipe_left,
    };
    for (const auto required_id : kRequired) {
        if (runtime.find_by_id(required_id) == nullptr) {
            std::cerr << "TransitionRuntime: required transition '" << required_id << "' is missing.\n";
            return false;
        }
    }
 
    std::cout << "Transition Runtime tests passed! (" << runtime_ids.size()
              << " runtime, " << preset_ids.size() << " presets)" << std::endl;
    return true;
}
