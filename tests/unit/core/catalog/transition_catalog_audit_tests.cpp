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

    TransitionRegistry transition_registry;
    tachyon::register_builtin_transitions(transition_registry);
    auto& preset_registry = tachyon::presets::TransitionPresetRegistry::instance();

    // Test 1: Registry has entries
    {
        auto ids = transition_registry.list_all_ids();
        check_true(!ids.empty(), "TransitionRegistry has entries");
    }

    // Test 2: No duplicate IDs
    {
        auto ids = transition_registry.list_all_ids();
        std::size_t original_count = ids.size();
        
        // Check for duplicates by using a set
        std::set<std::string> unique_ids(ids.begin(), ids.end());
        check_true(unique_ids.size() == original_count, "No duplicate IDs in registry");
    }

    // Test 3: All preset IDs exist in registry
    {
        auto preset_ids = preset_registry.list_ids();
        int missing_from_registry = 0;
        for (const auto& id : preset_ids) {
            if (!transition_registry.resolve(id)) {
                ++missing_from_registry;
                std::cerr << "  Preset not in registry: " << id << '\n';
            }
        }
        check_true(missing_from_registry == 0, "All public presets are registered");
    }

    // Test 4: Resolve by ID works
    {
        auto ids = transition_registry.list_all_ids();
        if (!ids.empty()) {
            const auto* desc = transition_registry.resolve(ids[0]);
            check_true(desc != nullptr, "Resolve finds first entry");
            if (desc) {
                check_true(desc->id == ids[0], "Resolved ID matches");
            }
        }
    }

    // Test 5: Resolve by alias works (if any aliases exist)
    {
        auto descriptors = transition_registry.list_all();
        bool found_alias_test = false;
        for (const auto* desc : descriptors) {
            if (!desc->aliases.empty()) {
                const auto* by_alias = transition_registry.find_by_alias(desc->aliases[0]);
                check_true(by_alias != nullptr, "Find by alias works");
                if (by_alias) {
                    check_true(by_alias->id == desc->id, "Alias resolves to correct ID");
                }
                found_alias_test = true;
                break;
            }
        }
        if (!found_alias_test) {
            std::cout << "  No aliases found to test (ok)\n";
        }
    }

    std::cout << "Transition registry audit tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
