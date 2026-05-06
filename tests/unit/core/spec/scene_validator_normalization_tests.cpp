#include "tachyon/core/spec/validation/scene_validator.h"
#include "tachyon/core/spec/validation/layer_spec_normalizer.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"

#include <iostream>
#include <string>

namespace {

int g_failures = 0;

void check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_failures;
        std::cerr << "FAIL: " << message << '\\n';
    }
}

}  // namespace

bool run_scene_validator_normalization_tests() {
    g_failures = 0;

    using namespace tachyon;
    using namespace tachyon::core;

    // Test 1: Validator accepts canonical LayerType
    {
        SceneSpec scene;
        scene.project.id = "test_scene";

        CompositionSpec comp;
        comp.id = "comp1";

        LayerSpec layer;
        layer.id = "layer1";
        layer.type = LayerType::Text;
        layer.text_content = "Hello";
        layer.in_point = 0.0;
        layer.out_point = 5.0;

        comp.layers.push_back(layer);
        scene.compositions.push_back(comp);

        core::SceneValidator validator;
        auto result = validator.validate(scene);

        check_true(result.is_valid(), "Scene with canonical LayerType::Text passes validation");
    }

    std::cout << "Scene validator normalization tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\\n";

    return g_failures == 0;
}
