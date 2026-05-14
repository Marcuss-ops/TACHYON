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
#include <filesystem>
#include <vector>
#include <cstdio>

namespace fs = std::filesystem;

namespace tachyon {
TransitionRegistry create_default_transition_registry();
}

namespace tachyon::renderer2d {

void generate_transition_gallery() {
    std::cout << "[GALLERY] Starting Full HD (1080p) Video Gallery Generation with Procedural Aura...\n";
    
    fs::path base_output = "/home/pierone/Pyt/Tachyon/output";
    if (!fs::exists(base_output)) {
        fs::create_directories(base_output);
    }

    TransitionRegistry trans_reg = tachyon::create_default_transition_registry();
    auto all_transitions = trans_reg.list_all();

    EffectRegistry effect_reg;
    render::RenderIntent intent;
    RenderPlan plan;

    const int W = 1920;
    const int H = 1080;
    const int FPS = 30;
    const int DURATION_SEC = 2;
    const int TOTAL_FRAMES = FPS * DURATION_SEC;

    for (const auto* desc : all_transitions) {
        if (!desc) continue;
        
        std::cout << "[PROCESS] Rendering " << desc->id << "...\n";
        
        fs::path trans_dir = base_output / (desc->id + "_frames");
        fs::create_directories(trans_dir);

        SceneSpec scene;
        CompositionSpec comp;
        comp.id = "comp";
        comp.width = W;
        comp.height = H;

        // Layer 1: Aura Blueish (Background)
        LayerSpec l1;
        l1.identity.id = "bg";
        l1.identity.type = LayerType::Procedural;
        l1.source.procedural = ProceduralSpec{};
        l1.source.procedural->kind = "aura";
        l1.source.procedural->color_a.keyframes = {{0.0, {10, 10, 50, 255}}};
        l1.source.procedural->color_b.keyframes = {{0.0, {80, 80, 240, 255}}};
        l1.source.procedural->frequency.keyframes = {{0.0, 8.0}};
        l1.source.procedural->seed = 123;
        l1.playback.timing.start = 0.0;
        l1.playback.timing.duration = (double)DURATION_SEC;
        l1.transform.width = W;
        l1.transform.height = H;
        
        // Layer 2: Aura Golden (Foreground with Transition)
        LayerSpec l2;
        l2.identity.id = "fg";
        l2.identity.type = LayerType::Procedural;
        l2.source.procedural = ProceduralSpec{};
        l2.source.procedural->kind = "aura";
        l2.source.procedural->color_a.keyframes = {{0.0, {60, 40, 10, 255}}};
        l2.source.procedural->color_b.keyframes = {{0.0, {240, 200, 40, 255}}};
        l2.source.procedural->frequency.keyframes = {{0.0, 12.0}};
        l2.source.procedural->seed = 456;
        l2.playback.timing.start = 0.0;
        l2.playback.timing.duration = (double)DURATION_SEC;
        l2.transform.width = W;
        l2.transform.height = H;
        
        l2.transition_in.transition_id = desc->id;
        l2.transition_in.duration = (double)DURATION_SEC;

        comp.layers.push_back(l1);
        comp.layers.push_back(l2);
        scene.compositions.push_back(comp);

        for (int i = 0; i < TOTAL_FRAMES; ++i) {
            double t = (double)i / (double)FPS;
            auto evaluated = scene::evaluate_scene_composition_state(scene, "comp", t);
            
            RenderContext context;
            context.transition_registry = &trans_reg;
            
            FrameRenderTask task;
            task.time_seconds = t;
            task.frame_number = i;
            
            auto result = render_evaluated_composition_2d(*evaluated, intent, plan, task, context, effect_reg);
            
            char buf[128];
            std::sprintf(buf, "frame_%03d.png", i);
            fs::path frame_path = trans_dir / buf;
            result.surface->save_png(frame_path);
        }
        
        fs::path video_path = base_output / (desc->id + ".mp4");
        std::string cmd = "ffmpeg -y -i " + trans_dir.string() + "/frame_%03d.png -c:v libx264 -preset ultrafast -crf 15 -pix_fmt yuv420p " + video_path.string() + " > /dev/null 2>&1";
        
        std::system(cmd.c_str());
        fs::remove_all(trans_dir);
    }
    
    std::cout << "[GALLERY] Generation Complete.\n";
}

} // namespace tachyon::renderer2d
