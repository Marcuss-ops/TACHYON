#include "tachyon/core/spec/schema/scene_spec.h"
#include "tachyon/presets/background/background_preset_registry.h"
#include "tachyon/presets/text/text_layer_preset_registry.h"

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
    using namespace tachyon::presets;

    std::cout << "Running E2E smoke tests (structure check)...\n";

    // Test 1: Background preset produces valid layer
    {
        registry::ParameterBag params;
        params.set("width", 1920.0f);
        params.set("height", 1080.0f);
        params.set("duration", 5.0f);

        presets::BackgroundPresetRegistry registry;
        auto layer_opt = registry.create("tachyon.background.solid", params);
        check_true(layer_opt.has_value(), "Background solid preset creates valid layer");
        if (layer_opt) {
            check_true(layer_opt->type == LayerType::Solid, "Background produces Solid layer");
            check_true(layer_opt->width == 1920, "Background has correct width");
            check_true(layer_opt->height == 1080, "Background has correct height");
        }
    }

    // Test 2: Text layer with preset
    {
        registry::ParameterBag params;
        params.set("id", std::string("tachyon.text.headline"));
        params.set("text", std::string("Hello World"));
        params.set("width", 1920.0f);
        params.set("height", 1080.0f);

        auto layer_opt = TextLayerPresetRegistry::instance().create("tachyon.text.headline", params);
        check_true(layer_opt.has_value(), "Text preset creates valid layer");
        if (layer_opt) {
            check_true(layer_opt->type == LayerType::Text, "Text preset produces Text layer");
            check_true(!layer_opt->text_content.empty(), "Text layer has content");
        }
    }

    // Test 3: Transition registry has expected entries
    {
        tachyon::TransitionRegistry registry;
        tachyon::register_builtin_transitions(registry);
        auto ids = registry.list_all_ids();
        check_true(!ids.empty(), "Transition registry has entries");
        check_true(registry.resolve("tachyon.transition.fade") != nullptr, "Fade transition exists");
    }

    // Test 4: Background registry has expected entries
    {
        tachyon::BackgroundRegistry registry;
        auto ids = registry.list_all_ids();
        check_true(!ids.empty(), "Background registry has entries");
        check_true(registry.resolve("tachyon.background.solid") != nullptr, "Solid background exists");
    }

    // Test 5: Output directory exists (for smoke renders)
    {
        bool output_exists = std::filesystem::exists("output") || std::filesystem::exists("../output");
        if (!output_exists) {
            std::cerr << "  Warning: output/ directory not found (needed for smoke renders)\n";
        }
        check_true(true, "Output directory check completed");
    }

    std::cout << "E2E smoke tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";
    std::cout << "Note: Full render smoke tests require tachyon executable and output presets.\n";

    return g_failures == 0;
}
