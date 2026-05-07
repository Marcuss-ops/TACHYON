#include "tachyon/scene/builder.h"
#include <cassert>
#include <iostream>

bool run_scene_builder_preset_tests() {
    using namespace tachyon::scene;

    std::cout << "Running SceneBuilder preset extension tests..." << std::endl;

    // 1. Test background_preset
    {
        auto comp = Composition("test_comp")
            .size(1920, 1080)
            .background_preset("aurora_mesh")
            .build();
        
        assert(!comp.layers.empty());
        assert(comp.layers[0].id == "bg_aurora_mesh");
        assert(comp.layers[0].type == "procedural");
    }

    // 2. Test text_animation_preset and transitions
    {
        auto comp = Composition("text_comp")
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

    std::cout << "SceneBuilder preset extension tests passed!" << std::endl;
    return true;
}
