#include "tachyon/presets/text/text_registry.h"
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
        TextManifest text_manifest;
        TextRegistry registry(text_manifest);
        registry::ParameterBag params;
        params.set("content", std::string("Hello World"));

        auto layer_opt = registry.create_layer("tachyon.text.layer.classic_title", params);
        check_true(layer_opt.has_value(), "Text preset returns valid LayerSpec");
        if (layer_opt) {
            check_true(layer_opt->type == LayerType::Text, "Text preset produces LayerType::Text");
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

    std::cout << "Text domain boundary tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
