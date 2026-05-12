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

    // 3. Test camera defaults and overrides through the 2D builder path
    {
        auto comp = Composition("camera_comp")
            .layer("cam", [](LayerBuilder& l) {
                l.type(LayerType::Camera);
                l.camera().type("two_node").done();
            })
            .build();

        assert(comp.layers.size() == 1);
        const auto& layer = comp.layers[0];
        assert(layer.type == LayerType::Camera);
        assert(layer.camera_type == "two_node");
        assert(!layer.camera_poi.value.has_value());
        assert(!layer.camera_zoom.value.has_value());
    }

    {
        auto comp = Composition("camera_override_comp")
            .layer("cam", [](LayerBuilder& l) {
                l.type(LayerType::Camera);
                l.camera().type("one_node").poi(0.0, 0.0, 0.0).zoom(877.0).done();
            })
            .build();

        assert(comp.layers.size() == 1);
        const auto& layer = comp.layers[0];
        assert(layer.type == LayerType::Camera);
        assert(layer.camera_type == "one_node");
        assert(layer.camera_poi.value.has_value());
        assert(layer.camera_zoom.value.has_value());
        assert(layer.camera_poi.value->x == 0.0f);
        assert(layer.camera_poi.value->y == 0.0f);
        assert(layer.camera_poi.value->z == 0.0f);
        assert(layer.camera_zoom.value.value() == 877.0);
    }

    std::cout << "SceneBuilder preset extension tests passed!" << std::endl;
    return true;
}
