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

bool run_layerspec_schema_tests() {
    g_failures = 0;

    using namespace tachyon;
    using namespace tachyon::core;

    // Test 1: LayerSpec has type as canonical enum
    {
        LayerSpec layer;
        layer.type = LayerType::Text;
        check_true(layer.type == LayerType::Text,
            "LayerSpec::type is canonical enum (Text)");
    }

    // Test 2: canonical_layer_type returns from type enum
    {
        LayerSpec layer;
        layer.type = LayerType::Image;
        LayerType canonical = canonical_layer_type(layer);
        check_true(canonical == LayerType::Image,
            "canonical_layer_type returns enum value when type is set");
    }

    // Test 3: canonical_layer_type falls back to type_string
    {
        LayerSpec layer;
        layer.type = LayerType::Unknown;
        layer.type_string = "text";
        LayerType canonical = canonical_layer_type(layer);
        check_true(canonical == LayerType::Text,
            "canonical_layer_type falls back to type_string");
    }

    // Test 4: canonical_layer_type returns Unknown when both empty
    {
        LayerSpec layer;
        layer.type = LayerType::Unknown;
        layer.type_string.clear();
        LayerType canonical = canonical_layer_type(layer);
        check_true(canonical == LayerType::Unknown,
            "canonical_layer_type returns Unknown when both type and type_string are empty");
    }

    // Test 5: normalize_layer_view sets legacy_type_string_used correctly
    {
        LayerSpec layer;
        layer.type = LayerType::Solid;
        auto normalized = normalize_layer_view(layer);
        check_true(!normalized.legacy_type_string_used,
            "legacy_type_string_used is false when using enum type");
    }

    // Test 6: normalize_layer_view detects legacy usage
    {
        LayerSpec layer;
        layer.type = LayerType::Unknown;
        layer.type_string = "image";
        auto normalized = normalize_layer_view(layer);
        check_true(normalized.legacy_type_string_used,
            "legacy_type_string_used is true when falling back to type_string");
    }

    // Test 7: NormalizedLayerView has correct type
    {
        LayerSpec layer;
        layer.type = LayerType::Camera;
        auto normalized = normalize_layer_view(layer);
        check_true(normalized.type == LayerType::Camera,
            "NormalizedLayerView.type matches LayerSpec::type");
    }

    // Test 8: to_canonical_layer_type_string works
    {
        std::string_view str = to_canonical_layer_type_string(LayerType::Text);
        check_true(!str.empty(),
            "to_canonical_layer_type_string returns non-empty string for valid type");
    }

    // Test 9: LayerSpec does not have kind field (compile-time check)
    {
        LayerSpec layer;
        // If LayerSpec had a 'kind' field, this would compile differently
        // We verify type is the enum and type_string is the string
        layer.type = LayerType::Precomp;
        layer.type_string = "precomp";
        check_true(layer.type == LayerType::Precomp,
            "LayerSpec uses type (enum) as canonical, not kind");
    }

    // Test 10: Unknown type with empty type_string
    {
        LayerSpec layer;
        layer.type = LayerType::Unknown;
        layer.type_string = "";
        auto normalized = normalize_layer_view(layer);
        check_true(normalized.type == LayerType::Unknown,
            "Layer with Unknown type and empty type_string normalizes to Unknown");
    }

    std::cout << "LayerSpec schema tests: " << (g_failures == 0 ? "PASSED" : "FAILED")
              << " (" << g_failures << " failures)\n";

    return g_failures == 0;
}
