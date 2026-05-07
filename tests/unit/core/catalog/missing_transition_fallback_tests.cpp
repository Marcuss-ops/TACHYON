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

    TransitionRegistry registry;
    tachyon::register_builtin_transitions(registry);

    // Test 1: resolve returns null for fake transition
    {
        const auto* entry = registry.resolve("tachyon.transition.fake");
        check_true(entry == nullptr, "Fake transition ID returns null");
    }

    // Test 2: resolve returns null for fake runtime
    {
        const auto* entry = registry.find_by_id("fake_runtime_id");
        check_true(entry == nullptr, "Fake runtime ID returns null");
    }

    // Test 3: Known transition resolves successfully
    {
        const auto* entry = registry.resolve("tachyon.transition.fade");
        check_true(entry != nullptr, "Known transition 'fade' resolves");
    }

    // Test 4: Known transition by alias resolves
    {
        const auto* entry = registry.find_by_alias("crossfade");
        check_true(entry != nullptr, "Transition alias 'crossfade' resolves");
        if (entry) {
            check_true(entry->id == "tachyon.transition.fade",
                "Alias 'crossfade' resolves to correct ID");
        }
    }

    // Test 5: Resolve by nonexistent ID returns null
    {
        const auto* entry = registry.resolve("tachyon.transition.nonexistent");
        check_true(entry == nullptr, "resolve returns null for nonexistent ID");
    }

    // Test 6: Find by nonexistent alias returns null
    {
        const auto* entry = registry.find_by_alias("nonexistent_alias");
        check_true(entry == nullptr, "find_by_alias returns null for nonexistent alias");
    }

    // Test 7: Find by ID works for valid entries
    {
        const auto* entry = registry.find_by_id("tachyon.transition.fade");
        check_true(entry != nullptr, "find_by_id finds 'fade' transition");
    }

    std::cout << "Missing transition fallback tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
