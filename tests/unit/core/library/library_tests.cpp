#include "tachyon/core/library/library.h"

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

bool run_library_tests() {
    g_failures = 0;

    const auto root = repo_root() / "assets" / "library";
    tachyon::TachyonLibrary library(root);
    
    // Note: If assets/library doesn't exist yet, ok() might be true if only C++ presets are loaded,
    // but register_transition_assets will just return early.
    check_true(library.ok(), "TachyonLibrary initializes correctly");
    
    // Verification: default library root must be assets/library, never studio/library
    const auto expected_default = std::filesystem::current_path() / "assets" / "library";
    tachyon::TachyonLibrary default_library("");
    check_true(default_library.root() == expected_default, "Default library root is assets/library");
    check_true(default_library.root().string().find("studio") == std::string::npos, "Default library root does not contain 'studio'");

    check_true(!library.scenes().empty(), "TachyonLibrary exposes C++ scenes even without disk assets");
    
    // Disk-dependent tests - only run if library exists
    if (std::filesystem::exists(root / "transitions" / "library.txt")) {
        check_true(!library.transitions().empty(), "TachyonLibrary exposes transitions from disk");
        
        const auto transition = library.find_transition("tachyon.transition.crossfade");
        check_true(transition.has_value(), "TachyonLibrary can find crossfade from disk");
        if (transition.has_value()) {
            const auto effect = library.build_transition_effect("tachyon.transition.crossfade", "layer_a", "layer_b", 0.5);
            check_true(effect.enabled, "Transition effect is enabled");
            check_true(effect.type == "tachyon.effect.transition.glsl", "Transition effect type is canonical");
        }
    } else {
        std::cout << "Skipping disk-based library tests (assets/library not found)\n";
    }

    const auto scene = library.find_scene("tachyon.scene.minimal_text");
    check_true(scene.has_value(), "TachyonLibrary can find the minimal text C++ scene");
    if (scene.has_value()) {
        check_true(scene->is_cpp_preset, "Minimal text is a C++ preset");
    }

    // Fallback test: register a transition with a non-existent shader
    const auto fallback_effect = library.build_transition_effect("non_existent_id", "a", "b", 0.5);
    
    const auto crossfade = library.find_transition("tachyon.transition.crossfade");
    if (crossfade.has_value()) {
        const auto effect = library.build_transition_effect("tachyon.transition.crossfade", "a", "b", 0.5);
        check_true(effect.enabled, "Crossfade effect is enabled (even with fallback)");
        check_true(!std::get<std::string>(effect.params.at("shader_path")).empty(), "Shader path is set (real or fallback)");
    }

    return g_failures == 0;
}
