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

bool run_missing_transition_fallback_tests() {
    g_failures = 0;

    auto& catalog = tachyon::TransitionCatalog::instance();

    // Test 1: validate_preset_transition returns false for fake transition in strict mode
    {
        std::string error;
        bool valid = catalog.validate_preset_transition("tachyon.transition.fake", error);
        check_false(valid, "Fake transition ID fails validation");
        check_false(error.empty(), "Error message set for fake transition");
    }

    // Test 2: validate_runtime_transition returns false for fake runtime
    {
        std::string error;
        bool valid = catalog.validate_runtime_transition("fake_runtime_id", error);
        check_false(valid, "Fake runtime ID fails validation");
        check_false(error.empty(), "Error message set for fake runtime");
    }

    // Test 3: Known transition validates successfully
    {
        std::string error;
        bool valid = catalog.validate_preset_transition("tachyon.transition.fade", error);
        check_true(valid, "Known transition 'fade' passes validation");
    }

    // Test 4: Known transition by alias validates
    {
        std::string error;
        bool valid = catalog.validate_preset_transition("crossfade", error);
        check_true(valid, "Transition alias 'crossfade' passes validation");
    }

    // Test 5: Search by alias works
    {
        const auto* entry = catalog.find_by_alias("crossfade");
        check_true(entry != nullptr, "find_by_alias finds 'crossfade'");
        if (entry) {
            check_true(entry->id == "tachyon.transition.fade",
                "Alias 'crossfade' resolves to correct ID");
        }
    }

    // Test 6: Unknown transition returns null from find
    {
        const auto* entry = catalog.find("tachyon.transition.nonexistent");
        check_true(entry == nullptr, "find returns null for nonexistent ID");
    }

    // Test 7: Unknown alias returns null
    {
        const auto* entry = catalog.find_by_alias("nonexistent_alias");
        check_true(entry == nullptr, "find_by_alias returns null for nonexistent alias");
    }

    // Test 8: Runtime ID lookup works for valid entries
    {
        const auto* entry = catalog.find_by_runtime_id("fade");
        check_true(entry != nullptr, "find_by_runtime_id finds 'fade' runtime");
    }

    std::cout << "Missing transition fallback tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
