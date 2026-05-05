#include "tachyon/presets/text/text_animator_preset_registry.h"
#include "tachyon/core/registry/parameter_bag.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

bool test_registry_is_not_empty() {
    auto& registry = tachyon::presets::TextAnimatorPresetRegistry::instance();
    if (registry.list_ids().empty()) {
        std::cerr << "Error: Registry is empty" << std::endl;
        return false;
    }
    return true;
}

bool test_common_presets_available() {
    auto& registry = tachyon::presets::TextAnimatorPresetRegistry::instance();
    std::vector<std::string> ids = {
        "tachyon.textanim.fade_in",
        "tachyon.textanim.slide_in",
        "tachyon.textanim.pop_in",
        "tachyon.textanim.typewriter",
        "tachyon.textanim.blur_to_focus"
    };
    for (const auto& id : ids) {
        if (registry.find(id) == nullptr) {
            std::cerr << "Error: Preset " << id << " not found" << std::endl;
            return false;
        }
    }
    return true;
}

bool test_create_preset_returns_valid_spec() {
    auto& registry = tachyon::presets::TextAnimatorPresetRegistry::instance();
    tachyon::registry::ParameterBag params;
    params.set("selector_based_on", std::string("characters_excluding_spaces"));
    params.set("char_delay", 0.03);
    params.set("duration", 0.7);

    auto specs = registry.create("tachyon.textanim.fade_in", params);
    if (specs.empty()) {
        std::cerr << "Error: Create tachyon.textanim.fade_in returned empty vector" << std::endl;
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
    auto& registry = tachyon::presets::TextAnimatorPresetRegistry::instance();
    tachyon::registry::ParameterBag params;
    params.set("selector_based_on", std::string("characters_excluding_spaces"));
    params.set("char_delay", 0.03);
    params.set("duration", 0.7);

    auto specs = registry.create("tachyon.textanim.fade_up", params);
    if (specs.size() != 3) {
        std::cerr << "Error: Composite tachyon.textanim.fade_up should return 3 specs, got " << specs.size() << std::endl;
        return false;
    }
    return true;
}

bool test_unknown_preset_falls_back() {
    auto& registry = tachyon::presets::TextAnimatorPresetRegistry::instance();
    tachyon::registry::ParameterBag params;
    auto specs = registry.create("tachyon.textanim.nonexistent_preset", params);
    if (specs.empty()) {
        std::cerr << "Error: Unknown preset fallback returned empty vector" << std::endl;
        return false;
    }
    if (specs.size() != 1) {
        std::cerr << "Error: Unknown preset should fallback to single fade_in spec, got " << specs.size() << std::endl;
        return false;
    }
    return true;
}

bool run_text_animator_preset_registry_tests() {
    bool success = true;

    std::cout << "Running TextAnimatorPresetRegistry tests..." << std::endl;

    if (!test_registry_is_not_empty()) success = false;
    if (!test_common_presets_available()) success = false;
    if (!test_create_preset_returns_valid_spec()) success = false;
    if (!test_composite_preset_returns_multiple_specs()) success = false;
    if (!test_unknown_preset_falls_back()) success = false;

    if (success) {
        std::cout << "TextAnimatorPresetRegistry tests passed!" << std::endl;
    } else {
        std::cout << "TextAnimatorPresetRegistry tests FAILED!" << std::endl;
    }

    return success;
}
