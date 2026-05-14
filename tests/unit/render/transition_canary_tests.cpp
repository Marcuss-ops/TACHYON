#include "tachyon/renderer2d/evaluated_composition/composition_renderer.h"
#include "tachyon/core/scene/evaluation/evaluator.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/render/render_intent.h"
#include "tachyon/renderer2d/effects/effect_registry.h"
#include "tachyon/core/transition/transition_resolver.h"
#include "tachyon/transition_registry.h"
#include <iostream>
#include <iomanip>

namespace tachyon {
// Declaration in the correct namespace
TransitionRegistry create_default_transition_registry();
}

namespace tachyon::renderer2d {

void test_crossfade_transition() {
    // 0. Create default transition registry
    TransitionRegistry trans_reg = tachyon::create_default_transition_registry();

    // 1. Setup a simple scene
    SceneSpec scene;
    CompositionSpec comp;
    comp.id = "comp";
    comp.width = 100;
    comp.height = 100;

    LayerSpec layer;
    layer.identity.id = "layer1";
    layer.identity.type = LayerType::Solid;
    layer.text.fill_color.keyframes = {{0.0, {255, 0, 0, 255}}}; // Red
    
    layer.playback.timing.start = 0.0;
    layer.playback.timing.duration = 2.0;
    
    layer.transition_in.transition_id = "tachyon.transition.crossfade";
    layer.transition_in.duration = 1.0;

    comp.layers.push_back(layer);
    scene.compositions.push_back(comp);

    // 2. Evaluate and Render at 0.5s
    auto evaluated = scene::evaluate_scene_composition_state(scene, "comp", 0.5);
    
    RenderContext context;
    context.transition_registry = &trans_reg;
    render::RenderIntent intent;
    RenderPlan plan;
    FrameRenderTask task;
    task.time_seconds = 0.5;
    task.frame_number = 15;
    
    EffectRegistry effect_reg;
    auto result = render_evaluated_composition_2d(*evaluated, intent, plan, task, context, effect_reg);

    // 3. Verify opacity
    auto color = result.surface->get_pixel(50, 50);
    
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Pixel at 50,50 during crossfade (progress 0.5): R=" << color.r << " G=" << color.g << " B=" << color.b << " A=" << color.a << "\n";
    
    if (color.a > 0.4f && color.a < 0.6f) {
        std::cout << "SUCCESS: Crossfade transition applied correctly!\n";
    } else {
        std::cerr << "FAIL: Crossfade transition alpha mismatch. Expected ~0.5, got " << color.a << "\n";
    }
}

} // namespace tachyon::renderer2d
