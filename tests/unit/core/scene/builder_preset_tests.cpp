#include "tachyon/scene/builder.h"
#include "tachyon/presets/preset_applier.h"
#include "tachyon/presets/effects/effect_preset_registry.h"
#include <cassert>
#include <iostream>

bool run_scene_builder_preset_tests() {
    using namespace tachyon;
    using namespace tachyon::scene;
    using namespace tachyon::presets;

    std::cout << "Running SceneBuilder preset extension tests..." << std::endl;

    // 1. Test background_preset (now via PresetApplier)
    {
        auto cb = Composition("test_comp");
        cb.size(1920, 1080);
        
        CompositionSpec spec = cb.build();
        PresetApplier::apply_background(spec, "aurora_mesh");
        
        assert(!spec.layers.empty());
        assert(spec.layers[0].id == "bg_aurora_mesh");
        assert(spec.layers[0].type == LayerType::Procedural);
    }

    // 2. Test text_animation_preset and transitions (now via PresetApplier)
    {
        auto cb = Composition("text_comp");
        cb.size(1920, 1080);
        
        LayerSpec text_layer;
        text_layer.id = "title";
        text_layer.type = LayerType::Text;
        text_layer.text.content = "Hello World";
        
        // Applying preset and transitions
        PresetApplier::apply_text_animation(text_layer, "tachyon.textanim.fade_up");
        
        text_layer.transition_in.transition_id = "tachyon.transition.crossfade";
        text_layer.transition_in.duration = 0.5;
        text_layer.transition_out.transition_id = "tachyon.transition.zoom";
        text_layer.transition_out.duration = 0.3;
        
        cb.layer(text_layer);
        CompositionSpec comp = cb.build();
        
        assert(!comp.layers.empty());
        const auto& layer = comp.layers[0];
        assert(!layer.text_animators.empty());
        assert(layer.transition_in.transition_id == "tachyon.transition.crossfade");
        assert(layer.transition_in.duration == 0.5);
        assert(layer.transition_out.transition_id == "tachyon.transition.zoom");
        assert(layer.transition_out.duration == 0.3);
    }

    std::cout << "SceneBuilder preset extension tests passed!" << std::endl;
    return true;
}
