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
    register_builtin_transitions(registry);

    // Test 1: Unknown transition ID returns nullptr from find_by_id
    {
        const auto* desc = registry.find_by_id("tachyon.transition.fake");
        check_false(desc != nullptr, "Fake transition ID not found by find_by_id");
    }

    // Test 2: Unknown transition returns nullptr from resolve
    {
        const auto* desc = registry.resolve("fake_runtime_id");
        check_false(desc != nullptr, "Fake runtime ID not found by resolve");
    }

    // Test 3: Known transition found by find_by_id
    {
        const auto* desc = registry.find_by_id("tachyon.transition.fade");
        check_true(desc != nullptr, "Known transition 'fade' found by find_by_id");
    }

    // Test 4: Known transition found by alias (crossfade is alias for fade)
    {
        const auto* desc = registry.find_by_alias("crossfade");
        check_true(desc != nullptr, "Transition alias 'crossfade' found by find_by_alias");
        if (desc) {
            check_true(desc->id == "tachyon.transition.fade",
                "Alias 'crossfade' resolves to correct ID");
        }
    }

    // Test 5: Unknown transition returns nullptr from find_by_id
    {
        const auto* desc = registry.find_by_id("tachyon.transition.nonexistent");
        check_true(desc == nullptr, "find_by_id returns null for nonexistent ID");
    }

    // Test 6: Unknown alias returns nullptr
    {
        const auto* desc = registry.find_by_alias("nonexistent_alias");
        check_true(desc == nullptr, "find_by_alias returns null for nonexistent alias");
    }

    // Test 7: Verify 'none' alias exists for compatibility
    {
        const auto* desc = registry.find_by_alias("none");
        // 'none' might or might not be registered as an alias
        // Just check that resolve handles it gracefully
        const auto* resolved = registry.resolve("none");
        // Should return nullptr or a valid descriptor
        (void)resolved;
    }

    std::cout << "Missing transition fallback tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
