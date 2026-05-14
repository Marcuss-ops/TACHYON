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
        scene.schema_version = {1, 0, 0};
        scene.project.id = "test_scene";

        CompositionSpec comp;
        comp.id = "comp1";
        comp.width = 1920;
        comp.height = 1080;
        comp.fps = 30.0f;
        comp.duration = 5.0f;

        LayerSpec layer;
        layer.id = "layer1";
        layer.type = LayerType::Text;
        layer.text_content = "Hello";
        layer.timing.source_in = 0.0;
        layer.timing.source_out = 5.0;

        comp.layers.push_back(layer);
        scene.compositions.push_back(comp);

        core::SceneValidator validator;
        auto result = validator.validate(scene);

        if (!result.is_valid()) {
            for (const auto& issue : result.issues) {
                std::cerr << "Validation issue: " << issue.message << " at " << issue.path << "\n";
            }
        }

        check_true(result.is_valid(), "Scene with canonical LayerType::Text passes validation");
    }

    // Test 2: Validator rejects legacy type_string
    {
        SceneSpec scene;
        CompositionSpec comp;
        comp.id = "comp1";
        comp.width = 1920;
        comp.height = 1080;
        comp.duration = 5.0f;

        LayerSpec layer;
        layer.id = "layer2";
        layer.type = LayerType::Text;
        layer.type = LayerType::Image;
        layer.timing.source_in = 0.0;
        layer.timing.source_out = 5.0;

        comp.layers.push_back(layer);
        scene.compositions.push_back(comp);

        core::SceneValidator validator;
        auto result = validator.validate(scene);

        check_true(!result.is_valid(), "Scene with legacy type_string fails validation");
        bool found_expected_error = false;
        for (const auto& issue : result.issues) {
            if (issue.message.find("type_string is no longer accepted") != std::string::npos) {
                found_expected_error = true;
                break;
            }
        }
        check_true(found_expected_error, "Found expected error message for type_string");
    }

    std::cout << "Scene validator normalization tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\\n";

    return g_failures == 0;
}
