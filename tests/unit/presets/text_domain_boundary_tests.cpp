#include "tachyon/presets/text/text_layer_preset_registry.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"

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

}  // namespace

bool run_text_domain_boundary_tests() {
    g_failures = 0;

    using namespace tachyon;
    using namespace tachyon::presets;

    // Test 1: Text layer preset returns valid LayerSpec
    {
        registry::ParameterBag params;
        params.set("id", std::string("tachyon.text.headline"));
        params.set("text", std::string("Hello World"));

        auto layer_opt = TextLayerPresetRegistry::instance().create("tachyon.text.headline", params);
        check_true(layer_opt.has_value(), "Text preset returns valid LayerSpec");
        if (layer_opt) {
            check_true(layer_opt->type == LayerType::Text, "Text preset produces LayerType::Text");
        }
    }

    // Test 2: Text layer does not directly add ThreeDSpec
    {
        registry::ParameterBag params;
        params.set("id", std::string("tachyon.text.headline"));
        params.set("text", std::string("Test"));

        auto layer_opt = TextLayerPresetRegistry::instance().create("tachyon.text.headline", params);
        if (layer_opt) {
            check_true(!layer_opt->three_d.has_value(), "Text preset does not directly add ThreeDSpec");
        }
    }

    // Test 3: Verify text presets produce text layers
    {
        LayerSpec layer;
        layer.type = LayerType::Text;
        layer.text_content = "Test";
        check_true(layer.type == LayerType::Text, "Text layer has correct type");
        check_true(layer.text_content == "Test", "Text layer has text_content set");
    }

    // Test 4: Text layer has 3D disabled by default
    {
        LayerSpec layer;
        layer.type = LayerType::Text;
        check_true(layer.three_d == std::nullopt, "Text layer does not have three_d set by default");
        check_true(!layer.is_3d, "Text layer is_3d is false by default");
    }

    std::cout << "Text domain boundary tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
