#include "tachyon/scene/builder.h"
#include "tachyon/presets/effects/effect_preset_registry.h"
#include <cassert>
#include <iostream>

bool run_scene_builder_preset_tests() {
    using namespace tachyon;
    using namespace tachyon::scene;

    std::cout << "Running SceneBuilder preset extension tests..." << std::endl;

    tachyon::presets::EffectPresetRegistry effects;

    // 1. Test background_preset
    {
        auto comp = Composition("test_comp", effects)
            .size(1920, 1080)
            .background_preset("aurora_mesh")
            .build();
        
        assert(!comp.layers.empty());
        assert(comp.layers[0].id == "bg_aurora_mesh");
        assert(comp.layers[0].type == LayerType::Procedural);
    }

    // 2. Test text_animation_preset and transitions
    {
        auto comp = Composition("text_comp", effects)
            .size(1920, 1080)
            .layer("title", [](LayerBuilder& l) {
                l.text().content("Hello World")
                 .animation_preset("tachyon.textanim.fade_up").done()
                 .enter().id("tachyon.transition.crossfade").duration(0.5).done()
                 .exit().id("tachyon.transition.zoom").duration(0.3).done();
            })
            .build();
        
        assert(!comp.layers.empty());
        const auto& layer = comp.layers[0];
        assert(!layer.text_animators.empty());
        assert(layer.transition_in.type == "tachyon.transition.crossfade");
        assert(layer.transition_in.duration == 0.5);
        assert(layer.transition_out.type == "tachyon.transition.zoom");
        assert(layer.transition_out.duration == 0.3);
    }

    // 3. Test camera3d_layer default and overrides
    {
        auto comp = Composition("camera_comp")
            .camera3d_layer("cam", [](LayerBuilder& l) {
            })
            .build();

        assert(comp.layers.size() == 1);
        const auto& layer = comp.layers[0];
        assert(layer.type == LayerType::Camera);
        assert(layer.is_3d);
        assert(layer.camera_type == "two_node");
        assert(!layer.camera_poi.value.has_value());
        assert(!layer.camera_zoom.value.has_value());
    }

    {
        auto comp = Composition("camera_override_comp")
            .camera3d_layer("cam", [](LayerBuilder& l) {
                l.camera().type("one_node").poi(0.0, 0.0, 0.0).zoom(877.0).done();
            })
            .build();

        assert(comp.layers.size() == 1);
        const auto& layer = comp.layers[0];
        assert(layer.type == LayerType::Camera);
        assert(layer.is_3d);
        assert(layer.camera_type == "one_node");
        assert(layer.camera_poi.value.has_value());
        assert(layer.camera_zoom.value.has_value());
        assert(layer.camera_poi.value->x == 0.0f);
        assert(layer.camera_poi.value->y == 0.0f);
        assert(layer.camera_poi.value->z == 0.0f);
        assert(layer.camera_zoom.value.value() == 877.0);
    }

    // 4. Test from_spec followed by 3D helpers keeps both payloads.
    {
        auto comp = Composition("from_spec_comp")
            .layer("title", [](LayerBuilder& l) {
                LayerSpec preset;
                preset.id = "title";
                preset.name = "Title";
                preset.type = LayerType::Text;
                preset.text_content = "TEST";

                l.from_spec(preset);
                l.position3d(1.0, 2.0, 3.0)
                 .rotation3d(10.0, 20.0, 30.0)
                 .extrude3d(0.4)
                 .bevel3d(0.05);
            })
            .build();

        assert(comp.layers.size() == 1);
        const auto& layer = comp.layers[0];
        assert(layer.type == LayerType::Text);
        assert(layer.text_content == "TEST");
        assert(layer.is_3d);
        assert(layer.transform3d.position_property.value.has_value());
        assert(layer.transform3d.rotation_property.value.has_value());
        assert(layer.three_d.has_value());
        assert(layer.three_d->enabled);
        assert(layer.three_d->extrusion_depth == 0.4);
        assert(layer.three_d->bevel_size == 0.05);
    }

    // 5. Test from_spec after 3D helpers resets the 3D payload.
    {
        auto comp = Composition("from_spec_reset_comp")
            .layer("title", [](LayerBuilder& l) {
                l.position3d(1.0, 2.0, 3.0);
                LayerSpec preset;
                preset.id = "title";
                preset.name = "Title";
                preset.type = LayerType::Text;
                preset.text_content = "RESET";

                l.from_spec(preset);
            })
            .build();

        assert(comp.layers.size() == 1);
        const auto& layer = comp.layers[0];
        assert(layer.type == LayerType::Text);
        assert(layer.text_content == "RESET");
        assert(!layer.is_3d);
        assert(!layer.transform3d.position_property.value.has_value());
        assert(!layer.three_d.has_value());
    }

    std::cout << "SceneBuilder preset extension tests passed!" << std::endl;
    return true;
}
