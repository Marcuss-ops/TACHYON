#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/validation/layer_spec_normalizer.h"
#include "tachyon/core/spec/schema/common/common_spec.h"

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

namespace tachyon {

bool run_layerspec_schema_tests() {
    g_failures = 0;

    using namespace tachyon;
    // using namespace tachyon::core; // Removed to avoid ambiguity

    // Test 1: LayerSpec has type as canonical enum in identity
    {
        LayerSpec layer;
        layer.identity.type = LayerType::Text;
        check_true(layer.identity.type == LayerType::Text,
            "LayerSpec::identity.type is canonical enum (Text)");
    }

    // Test 2: canonical_layer_type returns from identity.type enum
    {
        LayerSpec layer;
        layer.identity.type = LayerType::Image;
        LayerType canonical = canonical_layer_type(layer);
        check_true(canonical == LayerType::Image,
            "canonical_layer_type returns enum value when identity.type is set");
    }

    // Test 3: canonical_layer_type returns Unknown when identity.type is Unknown
    {
        LayerSpec layer;
        layer.identity.type = LayerType::Unknown;
        LayerType canonical = canonical_layer_type(layer);
        check_true(canonical == LayerType::Unknown,
            "canonical_layer_type returns Unknown when identity.type is Unknown");
    }

    // Test 4: normalize_layer_view works with enum type
    {
        LayerSpec layer;
        layer.identity.type = LayerType::Solid;
        auto normalized = core::normalize_layer_view(layer);
        check_true(normalized.type == LayerType::Solid,
            "normalize_layer_view preserves enum type");
    }

    // Test 5: NormalizedLayerView has correct type
    {
        LayerSpec layer;
        layer.identity.type = LayerType::Mask;
        auto normalized = core::normalize_layer_view(layer);
        check_true(normalized.type == LayerType::Mask,
            "NormalizedLayerView.type matches LayerSpec::identity.type");
    }

    // Test 6: to_canonical_layer_type_string works
    {
        std::string_view str = to_canonical_layer_type_string(LayerType::Text);
        check_true(!str.empty(),
            "to_canonical_layer_type_string returns non-empty string for valid type");
    }

    // Test 7: LayerSpec uses identity.type (enum) as canonical
    {
        LayerSpec layer;
        layer.identity.type = LayerType::Precomp;
        check_true(layer.identity.type == LayerType::Precomp,
            "LayerSpec uses identity.type (enum) as canonical");
    }

    // Test 8: Unknown type normalizes to Unknown
    {
        LayerSpec layer;
        layer.identity.type = LayerType::Unknown;
        auto normalized = core::normalize_layer_view(layer);
        check_true(normalized.type == LayerType::Unknown,
            "Layer with Unknown type normalizes to Unknown");
    }

    std::cout << "LayerSpec schema tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}

} // namespace tachyon
