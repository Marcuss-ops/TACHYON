#include "render_test_utils.h"
#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/render/render_intent.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/core/transition/transition_resolver.h"
#include "tachyon/transition_registry.h"
#include <iostream>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace tachyon {
TransitionRegistry create_default_transition_registry();
}

namespace tachyon::renderer2d {

using namespace tachyon::scene;

void generate_transition_gallery() {
    std::cout << "[GALLERY] Starting Full HD (1080p) Video Gallery Generation with Robust Pipeline...\n";
    
    fs::path base_output = fs::current_path() / "output" / "gallery";
    if (!fs::exists(base_output)) {
        fs::create_directories(base_output);
    }

    TransitionRegistry trans_reg = tachyon::create_default_transition_registry();
    auto all_transitions = trans_reg.list_all();

    for (const auto* desc : all_transitions) {
        if (!desc) continue;
        
        std::cout << "\n[PROCESS] Rendering " << desc->id << "...\n";
        
        SceneSpec scene;
        CompositionSpec comp;
        comp.id = "comp";
        comp.width = RobustRenderConfig::DefaultWidth;
        comp.height = RobustRenderConfig::DefaultHeight;

        // Layer 1: Aura Blueish (Background)
        LayerSpec l1;
        l1.identity.id = "bg";
        l1.identity.type = LayerType::Procedural;
        ProceduralSpec ps1;
        ps1.kind = "aura";
        ps1.color_a.keyframes = {{0.0, {10, 10, 50, 255}}};
        ps1.color_b.keyframes = {{0.0, {80, 80, 240, 255}}};
        ps1.frequency.keyframes = {{0.0, 8.0}};
        ps1.seed = 123;
        l1.source = ProceduralSource{ps1.kind, ps1};
        l1.playback.timing.start = 0.0;
        l1.playback.timing.duration = RobustRenderConfig::DefaultDuration;
        l1.transform.width = RobustRenderConfig::DefaultWidth;
        l1.transform.height = RobustRenderConfig::DefaultHeight;
        
        // Layer 2: Aura Golden (Foreground with Transition)
        LayerSpec l2;
        l2.identity.id = "fg";
        l2.identity.type = LayerType::Procedural;
        ProceduralSpec ps2;
        ps2.kind = "aura";
        ps2.color_a.keyframes = {{0.0, {60, 40, 10, 255}}};
        ps2.color_b.keyframes = {{0.0, {240, 200, 40, 255}}};
        ps2.frequency.keyframes = {{0.0, 12.0}};
        ps2.seed = 456;
        l2.source = ProceduralSource{ps2.kind, ps2};
        l2.playback.timing.start = 0.0;
        l2.playback.timing.duration = RobustRenderConfig::DefaultDuration;
        l2.transform.width = RobustRenderConfig::DefaultWidth;
        l2.transform.height = RobustRenderConfig::DefaultHeight;
        
        l2.transition_in.transition_id = desc->id;
        l2.transition_in.duration = RobustRenderConfig::DefaultDuration;

        comp.layers.push_back(l1);
        comp.layers.push_back(l2);
        scene.compositions.push_back(comp);

        fs::path video_path = base_output / (desc->id + ".mp4");
        
        RobustRenderConfig config;
        render_scene_to_mp4(scene, "comp", video_path.string(), trans_reg, config);
    }
    
    std::cout << "\n[GALLERY] Generation Complete.\n";
}

} // namespace tachyon::renderer2d
