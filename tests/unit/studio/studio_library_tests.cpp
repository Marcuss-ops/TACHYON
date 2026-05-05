#include "tachyon/studio/studio_library.h"

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

bool run_studio_library_tests() {
    g_failures = 0;

    const auto root = repo_root() / "studio" / "library";
    tachyon::StudioLibrary library(root);
    check_true(library.ok(), "StudioLibrary loads the public manifest");
    check_true(!library.scenes().empty(), "StudioLibrary exposes scenes");
    check_true(!library.transitions().empty(), "StudioLibrary exposes transitions");

    const auto transition = library.find_transition("tachyon.transition.crossfade");
    check_true(transition.has_value(), "StudioLibrary can find crossfade");
    if (transition.has_value()) {
        check_true(std::filesystem::exists(transition->output_dir), "Transition output directory exists");
        check_true(std::filesystem::exists(transition->demo_path), "Transition demo config exists");
        const auto effect = library.build_transition_effect("tachyon.transition.crossfade", "layer_a", "layer_b", 0.5);
        check_true(effect.enabled, "Transition effect is enabled");
        check_true(effect.type == "tachyon.effect.transition.glsl", "Transition effect type is canonical");
        check_true(effect.strings.at("transition_id") == "tachyon.transition.crossfade", "Transition effect stores transition id");
        check_true(effect.strings.at("transition_to_layer_id") == "layer_b", "Transition effect stores target layer id");
    }
    
    check_true(library.find_transition("tachyon.transition.slide_up").has_value(), "StudioLibrary can find slide_up");
    check_true(library.find_transition("tachyon.transition.swipe_left").has_value(), "StudioLibrary can find swipe_left");

    const auto scene = library.find_scene("tachyon.scene.minimal_text");
    check_true(scene.has_value(), "StudioLibrary can find the minimal text scene");
    if (scene.has_value() && !scene->is_cpp_preset) {
        check_true(std::filesystem::exists(scene->path), "Scene path exists");
    }

    check_true(std::filesystem::exists(root / "system"), "Studio system directory exists");

    return g_failures == 0;
}
