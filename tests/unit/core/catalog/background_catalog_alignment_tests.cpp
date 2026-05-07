#include "tachyon/background_registry.h"
#include "tachyon/presets/background/background_preset_registry.h"

#include <iostream>
#include <string>
#include <set>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

}  // namespace

bool run_background_catalog_alignment_tests() {
    g_failures = 0;

    tachyon::BackgroundRegistry registry;
    tachyon::presets::BackgroundPresetRegistry preset_registry;

    // Test 1: Registry has entries with valid IDs
    {
        auto ids = registry.list_all_ids();
        bool all_valid = true;
        for (const auto& id : ids) {
            if (id.empty()) {
                all_valid = false;
                break;
            }
        }
        check_true(all_valid, "All registry entries have valid non-empty IDs");
        check_true(!ids.empty(), "Background registry has entries");
    }

    // Test 2: Every public preset has registry entry
    {
        auto preset_ids = preset_registry.list_ids();
        int missing_from_registry = 0;
        for (const auto& id : preset_ids) {
            if (!registry.resolve(id)) {
                ++missing_from_registry;
                std::cerr << "  Preset not in registry: " << id << '\n';
            }
        }
        check_true(missing_from_registry == 0,
            "All public background presets have registry entries");
    }

    // Test 3: Registry entries can be found by ID
    {
        auto ids = registry.list_all_ids();
        bool all_findable = true;
        for (const auto& id : ids) {
            if (!registry.find_by_id(id)) {
                all_findable = false;
                break;
            }
        }
        check_true(all_findable, "All registry IDs are findable");
    }

    // Test 4: Valid preset resolves successfully
    {
        auto* desc = registry.resolve("tachyon.background.solid");
        check_true(desc != nullptr, "Valid preset 'solid' resolves");
    }

    // Test 5: Invalid preset returns null
    {
        auto* desc = registry.resolve("tachyon.background.nonexistent");
        check_true(desc == nullptr, "Invalid preset ID returns null");
    }

    // Test 6: No duplicate IDs
    {
        auto ids = registry.list_all_ids();
        std::size_t original_count = ids.size();
        std::set<std::string> unique_ids(ids.begin(), ids.end());
        check_true(unique_ids.size() == original_count, "No duplicate IDs in registry");
    }

    // Test 7: Resolve by alias works (if any aliases exist)
    {
        auto descriptors = registry.list_all();
        bool found_alias_test = false;
        for (const auto* desc : descriptors) {
            if (!desc->aliases.empty()) {
                auto* by_alias = registry.find_by_alias(desc->aliases[0]);
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

    std::cout << "Background registry alignment tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
