#include "tachyon/scene/builder.h"
#include "tachyon/core/spec/schema/objects/scene_spec_core.h"
#include <iostream>
#include <string>

namespace {

int g_ae_failures = 0;

void ae_check_true(bool condition, const std::string& message) {
    if (!condition) {
        ++g_ae_failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool ae_nearly_equal(double a, double b) {
    return std::abs(a - b) < 1e-4;
}

} // namespace

bool run_ae_builder_tests() {
    g_ae_failures = 0;

    // Null Layer
    {
        auto comp = tachyon::scene::Composition("main")
            .null_layer("controller", [](tachyon::scene::LayerBuilder& l) {
                l.position(960, 540);
            })
            .build();

        ae_check_true(comp.layers.size() == 1, "Null layer: layers size should be 1");
        if (comp.layers.size() == 1) {
            const auto& layer = comp.layers[0];
            ae_check_true(layer.id == "controller", "Null layer: ID should be controller");
            ae_check_true(layer.type == tachyon::LayerType::NullLayer, "Null layer: kind should be NullLayer");
        }
    }

    // Precomp Layer
    {
        auto comp = tachyon::scene::Composition("main")
            .precomp_layer("title_precomp", "actual_comp_id", [](tachyon::scene::LayerBuilder& l) {
                l.position(100, 100);
            })
            .build();

        ae_check_true(comp.layers.size() == 1, "Precomp layer: layers size should be 1");
        if (comp.layers.size() == 1) {
            const auto& layer = comp.layers[0];
            ae_check_true(layer.id == "title_precomp", "Precomp layer: ID should be title_precomp");
            ae_check_true(layer.type == tachyon::LayerType::Precomp, "Precomp layer: kind should be Precomp");
            ae_check_true(layer.precomp_id.has_value() && *layer.precomp_id == "actual_comp_id", "Precomp layer: precomp_id should match");
        }
    }

    // Parenting & Adjustment & Motion Blur
    {
        auto comp = tachyon::scene::Composition("main")
            .layer("child", [](tachyon::scene::LayerBuilder& l) {
                l.parent("parent_id")
                 .adjustment()
                 .motion_blur(true);
            })
            .build();

        ae_check_true(comp.layers.size() == 1, "Builder enhancements: layers size should be 1");
        if (comp.layers.size() == 1) {
            const auto& layer = comp.layers[0];
            ae_check_true(layer.parent.has_value() && *layer.parent == "parent_id", "Parenting should be set");
            ae_check_true(layer.is_adjustment_layer, "Adjustment layer should be true");
            ae_check_true(layer.motion_blur, "Motion blur should be true");
        }
    }

    // Track Matte
    {
        auto comp = tachyon::scene::Composition("main")
            .layer("video", [](tachyon::scene::LayerBuilder& l) {
                l.track_matte("mask_layer", tachyon::TrackMatteType::Alpha);
            })
            .build();

        ae_check_true(comp.layers.size() == 1, "Track matte: layers size should be 1");
        if (comp.layers.size() == 1) {
            const auto& layer = comp.layers[0];
            ae_check_true(layer.track_matte_layer_id.has_value() && *layer.track_matte_layer_id == "mask_layer", "Track matte layer ID should match");
            ae_check_true(layer.track_matte_type == tachyon::TrackMatteType::Alpha, "Track matte type should be Alpha");
        }
    }

    // Expressions
    {
        auto comp = tachyon::scene::Composition("main")
            .layer("wiggle_box", [](tachyon::scene::LayerBuilder& l) {
                l.opacity(tachyon::scene::expr::wiggle(2.0, 50.0, 12345));
            })
            .layer("sin_box", [](tachyon::scene::LayerBuilder& l) {
                l.opacity(tachyon::scene::expr::sin_wave(1.0, 0.5));
            })
            .build();

        ae_check_true(comp.layers.size() == 2, "Expressions: layers size should be 2");
        
        // Wiggle
        const auto& wiggle_layer = comp.layers[0];
        ae_check_true(wiggle_layer.opacity_property.wiggle.enabled, "Wiggle should be enabled");
        ae_check_true(ae_nearly_equal(wiggle_layer.opacity_property.wiggle.frequency, 2.0), "Wiggle frequency should match");
        ae_check_true(ae_nearly_equal(wiggle_layer.opacity_property.wiggle.amplitude, 50.0), "Wiggle amplitude should match");
        ae_check_true(wiggle_layer.opacity_property.wiggle.seed == 12345, "Wiggle seed should match");

        // Sin Wave (sampled)
        const auto& sin_layer = comp.layers[1];
        ae_check_true(!sin_layer.opacity_property.keyframes.empty(), "Sin wave should have keyframes");
        if (!sin_layer.opacity_property.keyframes.empty()) {
            ae_check_true(ae_nearly_equal(sin_layer.opacity_property.keyframes[0].value, 0.0), "Sin wave starts at 0");
        }
    }

    return g_ae_failures == 0;
}
