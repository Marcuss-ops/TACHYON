#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/text/text_layer_preset_registry.h"
#include "tachyon/transition_registry.h"
#include "tachyon/presets/transition/transition_preset_registry.h"
#include "tachyon/background_registry.h"
#include "tachyon/backgrounds.hpp"
#include <iostream>
#include <string>
#include <filesystem>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

}  // namespace

bool run_e2e_smoke_tests() {
    g_failures = 0;

    using namespace tachyon;

    std::cout << "Running E2E smoke tests (structure check)...\n";

    // Test 1: Background preset produces valid layer
    {
        presets::BackgroundManifest bg_manifest;
        presets::BackgroundPresetRegistry registry(bg_manifest);
        registry::ParameterBag params;
        params.set("width", 1920.0f);
        params.set("height", 1080.0f);
        params.set("duration", 5.0f);

        auto layer_opt = registry.create("tachyon.background.solid", params);
        check_true(layer_opt.has_value(), "Background solid preset creates valid layer");
        if (layer_opt) {
            check_true(layer_opt->type == LayerType::Solid, "Background produces Solid layer");
        }
    }

    // Test 2: Text layer with preset
    {
        presets::TextManifest text_manifest;
        presets::TextLayerPresetRegistry registry(text_manifest);
        registry::ParameterBag params;
        params.set("id", std::string("tachyon.text.headline"));
        params.set("text", std::string("Hello World"));
        params.set("width", 1920.0f);
        params.set("height", 1080.0f);

        auto layer_opt = registry.create("tachyon.text.headline", params);
        check_true(layer_opt.has_value(), "Text preset creates valid layer");
        if (layer_opt) {
            check_true(layer_opt->type == LayerType::Text, "Text preset produces Text layer");
        }
    }

    // Test 3: Transition registry has expected entries
    {
        TransitionRegistry transition_registry;
        register_builtin_transitions(transition_registry);
        auto descriptors = transition_registry.list_all();
        check_true(descriptors.size() > 0, "Transition registry has entries");
        
        auto* fade_desc = transition_registry.resolve("tachyon.transition.fade");
        check_true(fade_desc != nullptr, "Fade transition exists");
    }

    // Test 4: Background registry has expected entries
    {
        BackgroundRegistry background_registry;
        register_builtin_background_descriptors(background_registry);
        auto ids = background_registry.list_all_ids();
        check_true(ids.size() > 0, "Background registry has entries");
        
        auto* solid_desc = background_registry.resolve("tachyon.background.solid");
        check_true(solid_desc != nullptr, "Solid background exists");
    }

    std::cout << "E2E smoke tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
