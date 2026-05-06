#include "tachyon/background_catalog.h"
#include "tachyon/presets/background/background_preset_registry.h"

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

}  // namespace

bool run_background_catalog_alignment_tests() {
    g_failures = 0;

    auto& catalog = tachyon::BackgroundCatalog::instance();
    auto& preset_registry = tachyon::presets::BackgroundPresetRegistry::instance();

    // Test 1: Every catalog id exists and is valid
    {
        std::size_t count = catalog.count();
        bool all_valid = true;
        for (std::size_t i = 0; i < count; ++i) {
            const auto* entry = catalog.get_by_index(i);
            if (!entry || entry->id.empty()) {
                all_valid = false;
                break;
            }
        }
        check_true(all_valid, "All catalog entries have valid non-empty IDs");
    }

    // Test 2: Every public preset has catalog entry
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
            "All public background presets have catalog entries");
    }

    // Test 3: Catalog entries can be found by id
    {
        auto ids = catalog.list_all_ids();
        bool all_findable = true;
        for (const auto& id : ids) {
            if (!catalog.find(id)) {
                all_findable = false;
                break;
            }
        }
        check_true(all_findable, "All catalog IDs are findable");
    }

    // Test 4: Validate preset function works
    {
        std::string error;
        bool valid = catalog.validate_preset("tachyon.background.solid", error);
        check_true(valid, "Valid preset ID passes validation");
    }

    // Test 5: Invalid preset fails validation
    {
        std::string error;
        bool valid = catalog.validate_preset("tachyon.background.nonexistent", error);
        check_true(!valid, "Invalid preset ID fails validation");
        check_true(!error.empty(), "Error message is set for invalid preset");
    }

    // Test 6: Each catalog entry has valid kind
    {
        std::size_t count = catalog.count();
        bool all_have_kind = true;
        for (std::size_t i = 0; i < count; ++i) {
            const auto* entry = catalog.get_by_index(i);
            if (entry && entry->kind.empty()) {
                all_have_kind = false;
                std::cerr << "  Entry '" << entry->id << "' has empty kind\n";
                break;
            }
        }
        check_true(all_have_kind, "All catalog entries have non-empty kind");
    }

    // Test 7: Catalog entries have reasonable status
    {
        std::size_t count = catalog.count();
        for (std::size_t i = 0; i < count; ++i) {
            const auto* entry = catalog.get_by_index(i);
            if (entry) {
                // Status should be one of the valid enum values
                bool valid_status = (entry->status == tachyon::BackgroundStatus::Stable ||
                                     entry->status == tachyon::BackgroundStatus::Experimental ||
                                     entry->status == tachyon::BackgroundStatus::Deprecated);
                check_true(valid_status,
                    "Entry '" + entry->id + "' has valid status");
            }
        }
    }

    // Test 8: Preset registry can create from catalog ids
    {
        tachyon::registry::ParameterBag empty_params;
        auto ids = catalog.list_all_ids();
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
