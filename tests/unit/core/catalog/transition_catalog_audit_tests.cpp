#include "tachyon/transition_catalog.h"
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

    auto& catalog = tachyon::TransitionCatalog::instance();
    auto& preset_registry = tachyon::presets::TransitionPresetRegistry::instance();
    auto& transition_registry = tachyon::TransitionRegistry::instance();

    // Test 1: Audit returns results structure
    {
        auto result = catalog.audit();
        // Just check it runs without crashing
        check_true(true, "TransitionCatalog::audit() executes without crash");
    }

    // Test 2: Every catalog.runtime_id exists in TransitionRegistry
    {
        auto result = catalog.audit();
        check_true(result.missing_runtime.empty(),
            "All catalog runtime_ids exist in TransitionRegistry");
    }

    // Test 3: No duplicate aliases
    {
        auto result = catalog.audit();
        check_true(result.duplicate_aliases.empty(),
            "No alias conflicts in catalog");
    }

    // Test 4: Catalog entries have valid runtime_id
    {
        std::size_t count = catalog.count();
        bool all_have_runtime = true;
        for (std::size_t i = 0; i < count; ++i) {
            const auto* entry = catalog.get_by_index(i);
            if (entry && entry->runtime_id.empty()) {
                all_have_runtime = false;
                break;
            }
        }
        check_true(all_have_runtime,
            "All catalog entries have non-empty runtime_id");
    }

    // Test 5: Preset registry and catalog alignment
    {
        auto preset_ids = preset_registry.list_ids();
        int missing_from_catalog = 0;
        for (const auto& id : preset_ids) {
            if (!catalog.find(id)) {
                ++missing_from_catalog;
                std::cerr << "  Preset not in catalog: " << id << '\n';
            }
        }
        check_true(missing_from_catalog == 0,
            "All public presets are cataloged");
    }

    // Test 6: Runtime registry and catalog alignment
    {
        auto runtime_ids = transition_registry.list_builtin_transition_ids();
        int missing_from_catalog = 0;
        for (const auto& id : runtime_ids) {
            const auto* catalog_entry = catalog.find_by_runtime_id(id);
            if (!catalog_entry) {
                ++missing_from_catalog;
                std::cerr << "  Runtime transition not cataloged: " << id << '\n';
            }
        }
        check_true(missing_from_catalog == 0,
            "All public runtime transitions are cataloged");
    }

    // Test 7: amber_sweep consistency (if it exists)
    {
        const std::string amber_id = "tachyon.transition.lightleak.amber_sweep";
        const auto* amber_catalog = catalog.find(amber_id);
        const auto* amber_preset = preset_registry.find(amber_id);
        const auto* amber_runtime = transition_registry.find(amber_id);

        bool in_any = (amber_catalog != nullptr) || (amber_preset != nullptr) || (amber_runtime != nullptr);
        if (in_any) {
            check_true(amber_catalog != nullptr, "amber_sweep in catalog if in any registry");
            check_true(amber_preset != nullptr, "amber_sweep in preset registry if in any registry");
            check_true(amber_runtime != nullptr, "amber_sweep in runtime registry if in any registry");
        } else {
            std::cout << "  amber_sweep not registered (ok, optional)\n";
        }
    }

    std::cout << "Transition catalog audit tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
