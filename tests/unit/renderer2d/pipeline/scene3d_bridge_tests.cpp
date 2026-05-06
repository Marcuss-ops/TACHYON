#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/3d/three_d_spec.h"

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

bool run_2d_3d_bridge_tests() {
    g_failures = 0;

    using namespace tachyon;

    // Test 1: LayerSpec has three_d optional field
    {
        LayerSpec layer;
        check_true(!layer.three_d.has_value(),
            "LayerSpec.three_d is nullopt by default");
    }

    // Test 2: Can set three_d on a layer
    {
        LayerSpec layer;
        ThreeDSpec three_d;
        three_d.enabled = true;
        layer.three_d = three_d;
        check_true(layer.three_d.has_value(),
            "LayerSpec.three_d can be set");
        check_true(layer.three_d->enabled,
            "ThreeDSpec.enabled is true after setting");
    }

    // Test 3: Text layer can consume ThreeDSpec
    {
        LayerSpec layer;
        layer.type = LayerType::Text;
        layer.text_content = "Hello";

        ThreeDSpec three_d;
        three_d.enabled = true;
        layer.three_d = three_d;

        check_true(layer.type == LayerType::Text,
            "Text layer type is preserved");
        check_true(layer.three_d.has_value(),
            "Text layer can have three_d spec");
    }

    // Test 4: Image layer can consume ThreeDSpec
    {
        LayerSpec layer;
        layer.type = LayerType::Image;
        layer.asset_id = "test.png";

        ThreeDSpec three_d;
        three_d.enabled = true;
        layer.three_d = three_d;

        check_true(layer.type == LayerType::Image,
            "Image layer type is preserved");
        check_true(layer.three_d.has_value(),
            "Image layer can have three_d spec");
    }

    // Test 5: Verify is_3d flag on layer
    {
        LayerSpec layer;
        layer.is_3d = true;
        check_true(layer.is_3d,
            "LayerSpec.is_3d flag can be set");
    }

    // Test 6: ThreeDSpec has expected fields
    {
        ThreeDSpec spec;
        // Just verify it compiles and has the expected structure
        spec.enabled = true;
        check_true(spec.enabled,
            "ThreeDSpec.enabled field is accessible");
    }

    std::cout << "2D/3D bridge tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
