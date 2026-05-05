#include "tachyon/core/catalog/catalog.h"

#include <filesystem>
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

std::filesystem::path repo_root() {
    return std::filesystem::path(TACHYON_TESTS_SOURCE_DIR).parent_path();
}

}  // namespace

bool run_catalog_tests() {
    g_failures = 0;

    const auto root = repo_root() / "assets" / "catalog";
    tachyon::TachyonCatalog catalog(root);
    
    // Note: If assets/catalog doesn't exist yet, ok() might be true if only C++ presets are loaded,
    // but register_transition_assets will just return early.
    check_true(catalog.ok(), "TachyonCatalog initializes correctly");
    
    // Verification: default catalog root must be assets/catalog, never studio/library
    const auto expected_default = std::filesystem::current_path() / "assets" / "catalog";
    tachyon::TachyonCatalog default_catalog("");
    check_true(default_catalog.root() == expected_default, "Default catalog root is assets/catalog");
    check_true(default_catalog.root().string().find("studio") == std::string::npos, "Default catalog root does not contain 'studio'");

    check_true(!catalog.scenes().empty(), "TachyonCatalog exposes C++ scenes even without disk assets");
    
    // Disk-dependent tests - only run if catalog exists
    if (std::filesystem::exists(root / "transitions" / "catalog.txt")) {
        check_true(!catalog.transitions().empty(), "TachyonCatalog exposes transitions from disk");
        
        const auto transition = catalog.find_transition("tachyon.transition.crossfade");
        check_true(transition.has_value(), "TachyonCatalog can find crossfade from disk");
        if (transition.has_value()) {
            const auto effect = catalog.build_transition_effect("tachyon.transition.crossfade", "layer_a", "layer_b", 0.5);
            check_true(effect.enabled, "Transition effect is enabled");
            check_true(effect.type == "tachyon.effect.transition.glsl", "Transition effect type is canonical");
        }
    } else {
        std::cout << "Skipping disk-based catalog tests (assets/catalog not found)\n";
    }

    const auto scene = catalog.find_scene("tachyon.scene.minimal_text");
    check_true(scene.has_value(), "TachyonCatalog can find the minimal text C++ scene");
    if (scene.has_value()) {
        check_true(scene->is_cpp_preset, "Minimal text is a C++ preset");
    }

    return g_failures == 0;
}
