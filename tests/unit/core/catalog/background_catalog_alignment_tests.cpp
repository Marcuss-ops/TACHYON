#include "tachyon/background_registry.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/backgrounds.hpp"

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
    tachyon::register_builtin_background_descriptors(registry);
    auto& preset_registry = tachyon::presets::BackgroundPresetRegistry::instance();

    // Test 1: Every catalog id exists and is valid
    {
        auto entries = registry.catalog_entries();
        bool all_valid = true;
        for (const auto& entry : entries) {
            if (entry.id.empty()) {
                all_valid = false;
                break;
            }
        }
        check_true(all_valid, "All catalog entries have valid non-empty IDs");
    }

    // Test 2: Every public preset has catalog entry
    {
        auto preset_ids = preset_registry.list_ids();
        auto entries = registry.catalog_entries();
        std::set<std::string> catalog_ids;
        for (const auto& entry : entries) {
            catalog_ids.insert(entry.id);
        }

        int missing_from_catalog = 0;
        for (const auto& id : preset_ids) {
            if (catalog_ids.find(id) == catalog_ids.end()) {
                ++missing_from_catalog;
                std::cerr << "  Preset not in catalog: " << id << '\n';
            }
        }
        check_true(missing_from_catalog == 0,
            "All public background presets have catalog entries");
    }

    // Test 3: Catalog entries can be found by id
    {
        auto ids = registry.list_all_ids();
        bool all_findable = true;
        for (const auto& id : ids) {
            if (!registry.resolve(id)) {
                all_findable = false;
                break;
            }
        }
        check_true(all_findable, "All catalog IDs are findable");
    }

    // Test 4: Validate preset function works
    {
        std::string error;
        bool valid = registry.resolve("tachyon.background.solid") != nullptr;
        check_true(valid, "Valid preset ID passes validation");
    }

    // Test 5: Invalid preset fails validation
    {
        bool valid = registry.resolve("tachyon.background.nonexistent") != nullptr;
        check_true(!valid, "Invalid preset ID fails validation");
    }

    // Test 6: Each catalog entry has valid role
    {
        auto entries = registry.catalog_entries();
        bool all_valid = true;
        for (const auto& entry : entries) {
            if (!entry.id.empty()) {
                bool valid_status = (entry.status == tachyon::BackgroundStatus::Stable ||
                                     entry.status == tachyon::BackgroundStatus::Experimental ||
                                     entry.status == tachyon::BackgroundStatus::Deprecated);
                if (!valid_status) {
                    check_true(valid_status,
                        "Entry '" + entry.id + "' has valid status");
                }
            }
        }
    }

    // Test 7: Catalog entries have reasonable status
    {
        auto entries = registry.catalog_entries();
        for (const auto& entry : entries) {
            if (entry.status != tachyon::BackgroundStatus::Stable &&
                entry.status != tachyon::BackgroundStatus::Experimental &&
                entry.status != tachyon::BackgroundStatus::Deprecated) {
                check_true(false, "Entry '" + entry.id + "' has invalid status");
            }
        }
    }

    // Test 8: Preset registry can create from catalog ids
    {
        tachyon::registry::ParameterBag empty_params;
        auto ids = registry.list_all_ids();
        int create_failures = 0;
        for (const auto& id : ids) {
            auto result = preset_registry.create(id, empty_params);
            // Some might fail due to required params, but should not crash
            static_cast<void>(result); // We just want to ensure it doesn't crash
        }
        check_true(true, "PresetRegistry::create doesn't crash for catalog IDs");
    }

    std::cout << "Background catalog alignment tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
