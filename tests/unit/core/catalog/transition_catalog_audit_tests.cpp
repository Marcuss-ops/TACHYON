#include "tachyon/transition_registry.h"
#include "tachyon/presets/transition/transition_preset_registry.h"

#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void check_false(bool condition, const std::string& message) {
    check_true(!condition, message);
}

}  // namespace

bool run_transition_catalog_audit_tests() {
    g_failures = 0;

    TransitionRegistry registry;
    register_builtin_transitions(registry);
    auto& preset_registry = tachyon::presets::TransitionPresetRegistry::instance();

    // Test 1: Registry is not empty after init
    {
        auto descriptors = registry.list_all();
        check_true(descriptors.size() > 0, "TransitionRegistry has entries after init");
    }

    // Test 2: Every registered transition has required fields
    {
        auto descriptors = registry.list_all();
        bool all_valid = true;
        for (const auto* desc : descriptors) {
            if (desc->id.empty()) {
                all_valid = false;
                break;
            }
        }
        check_true(all_valid, "All registered transitions have valid ids");
    }

    // Test 3: No duplicate ids
    {
        auto ids = registry.list_all_ids();
        bool has_dupes = false;
        for (size_t i = 0; i < ids.size(); ++i) {
            for (size_t j = i + 1; j < ids.size(); ++j) {
                if (ids[i] == ids[j]) {
                    has_dupes = true;
                    break;
                }
            }
            if (has_dupes) break;
        }
        check_true(!has_dupes, "No duplicate transition ids in registry");
    }

    // Test 4: Preset registry and runtime registry alignment
    {
        auto preset_ids = preset_registry.list_ids();
        int missing_from_registry = 0;
        for (const auto& id : preset_ids) {
            if (registry.find_by_id(id) == nullptr) {
                ++missing_from_registry;
                std::cerr << "  Preset not in registry: " << id << '\n';
            }
        }
        check_true(missing_from_registry == 0,
            "All public presets are in TransitionRegistry");
    }

    // Test 5: amber_sweep consistency (if it exists)
    {
        const std::string amber_id = "tachyon.transition.lightleak.amber_sweep";
        const auto* amber_desc = registry.find_by_id(amber_id);
        const auto* amber_preset = preset_registry.find(amber_id);

        if (amber_desc != nullptr || amber_preset != nullptr) {
            check_true(amber_desc != nullptr, "amber_sweep in registry if in any registry");
            check_true(amber_preset != nullptr, "amber_sweep in preset registry if in any registry");
        } else {
            std::cout << "  amber_sweep not registered (ok, optional)\n";
        }
    }

    std::cout << "Transition catalog audit tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
