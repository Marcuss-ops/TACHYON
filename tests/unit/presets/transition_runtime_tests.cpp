#include "tachyon/transition_registry.h"
#include "tachyon/core/transition/transition_descriptor.h"
#include "tachyon/renderer2d/effects/core/glsl_transition_effect.h"
#include "tachyon/presets/transition/transition_preset_registry.h"
#include <cassert>
#include <iostream>
#include <set>

bool run_transition_runtime_tests() {
    using namespace tachyon;

    std::cout << "Running Transition Runtime tests..." << std::endl;

    // Ensure all built-in implementations are loaded.
    tachyon::register_builtin_transitions();
    renderer2d::init_builtin_transitions();
    auto& runtime = TransitionRegistry::instance();

    // 1. Registry must not be empty after init.
    assert(runtime.count() > 0);

    // 2. Every registered runtime transition must have either a pixel function
    //    or a valid state type — no orphan entries.
    const auto runtime_ids = runtime.list_builtin_transition_ids();
    for (const auto& id : runtime_ids) {
        const TransitionSpec* spec = runtime.find(id);
        if (spec == nullptr) {
            std::cerr << "TransitionRuntime: id '" << id << "' listed but not findable.\n";
            return false;
        }

        const bool has_impl =
            spec->function != nullptr ||
            spec->state_type != TransitionSpec::Type::None ||
            !spec->cpu_fn_name.empty();

        if (!has_impl) {
            std::cerr << "TransitionRuntime: '" << id << "' has no pixel function, state type, or cpu_fn_name.\n";
            return false;
        }
    }

    // 3. Reverse audit: every preset that maps to a runtime id must resolve.
    //    (Forward direction already in preset_audit_tests. This catches the
    //    case where a runtime transition was removed but preset still references it.)
    auto& preset_registry = presets::TransitionPresetRegistry::instance();
    const auto preset_ids = preset_registry.list_ids();

    for (const auto& pid : preset_ids) {
        if (pid == "tachyon.transition.none") continue;

        registry::ParameterBag bag;
        LayerTransitionSpec spec = preset_registry.create(pid, bag);

        if (spec.transition_id.empty()) {
            std::cerr << "TransitionRuntime: preset '" << pid << "' produced empty transition_id.\n";
            return false;
        }

        const TransitionSpec* runtime_spec = runtime.find(spec.transition_id);
        if (runtime_spec == nullptr) {
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
    static constexpr const char* kRequired[] = {
        "tachyon.transition.crossfade",
        "tachyon.transition.luma_dissolve",
        "tachyon.transition.slide_up",
        "tachyon.transition.swipe_left",
    };
    for (const char* required_id : kRequired) {
        if (runtime.find(required_id) == nullptr) {
            std::cerr << "TransitionRuntime: required transition '" << required_id << "' is missing.\n";
            return false;
        }
    }

    std::cout << "Transition Runtime tests passed! (" << runtime_ids.size()
              << " runtime, " << preset_ids.size() << " presets)" << std::endl;
    return true;
}
