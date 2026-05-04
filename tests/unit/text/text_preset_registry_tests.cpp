#include "tachyon/text/animation/text_preset_registry.h"
#include "tachyon/text/animation/text_scene_presets.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <iostream>
#include <string>
#include <vector>

using namespace tachyon;
using namespace tachyon::text;

bool test_registry_is_not_empty() {
    auto& registry = TextPresetRegistry::instance();
    if (registry.count() == 0) {
        std::cerr << "Error: Registry is empty" << std::endl;
        return false;
    }
    return true;
}

bool test_common_presets_available() {
    auto& registry = TextPresetRegistry::instance();
    std::vector<std::string> ids = {"fade_in", "slide_in", "pop_in", "typewriter", "blur_to_focus"};
    for (const auto& id : ids) {
        if (registry.find(id) == nullptr) {
            std::cerr << "Error: Preset " << id << " not found" << std::endl;
            return false;
        }
    }
    return true;
}

bool test_create_preset_returns_valid_spec() {
    auto& registry = TextPresetRegistry::instance();
    auto specs = registry.create("fade_in", "characters_excluding_spaces", 0.03, 0.7);
    if (specs.empty()) {
        std::cerr << "Error: Create fade_in returned empty vector" << std::endl;
        return false;
    }
    if (specs[0].selector.based_on.empty()) {
        std::cerr << "Error: Created spec has empty selector based_on" << std::endl;
        return false;
    }
    if (specs[0].properties.opacity_keyframes.empty()) {
        std::cerr << "Error: Created spec has no opacity keyframes" << std::endl;
        return false;
    }
    return true;
}

bool test_composite_preset_returns_multiple_specs() {
    auto& registry = TextPresetRegistry::instance();
    auto specs = registry.create("fade_up", "characters_excluding_spaces", 0.03, 0.7);
    if (specs.size() != 3) {
        std::cerr << "Error: Composite fade_up should return 3 specs, got " << specs.size() << std::endl;
        return false;
    }
    return true;
}

bool test_unknown_preset_falls_back() {
    auto& registry = TextPresetRegistry::instance();
    auto specs = registry.create("nonexistent_preset");
    if (specs.empty()) {
        std::cerr << "Error: Unknown preset fallback returned empty vector" << std::endl;
        return false;
    }
    // Should fallback to fade_in (single spec)
    if (specs.size() != 1) {
        std::cerr << "Error: Unknown preset should fallback to single fade_in spec, got " << specs.size() << std::endl;
        return false;
    }
    return true;
}

bool run_text_preset_registry_tests() {
    bool success = true;
    
    std::cout << "Running TextPresetRegistry tests..." << std::endl;
    
    if (!test_registry_is_not_empty()) success = false;
    if (!test_common_presets_available()) success = false;
    if (!test_create_preset_returns_valid_spec()) success = false;
    if (!test_composite_preset_returns_multiple_specs()) success = false;
    if (!test_unknown_preset_falls_back()) success = false;
    
    if (success) {
        std::cout << "TextPresetRegistry tests passed!" << std::endl;
    } else {
        std::cout << "TextPresetRegistry tests FAILED!" << std::endl;
    }
    
    return success;
}
