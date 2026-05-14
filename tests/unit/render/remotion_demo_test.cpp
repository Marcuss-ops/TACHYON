#include "tachyon/scene/builder.h"
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
#include <cstdio>

namespace fs = std::filesystem;

namespace tachyon {
TransitionRegistry create_default_transition_registry();
}

namespace tachyon::renderer2d {

using namespace tachyon::scene;

void render_remotion_demo() {
    std::cout << "[DEMO] Rendering Procedural Remotion Demo...\n";
    
    fs::path base_output = fs::current_path() / "output" / "transitions";
    if (!fs::exists(base_output)) {
        fs::create_directories(base_output);
    }

    TransitionRegistry trans_reg = tachyon::create_default_transition_registry();
    EffectRegistry effect_reg;
    render::RenderIntent intent;
    RenderPlan plan;

    const int W = 1920;
    const int H = 1080;
    const int FPS = 30;
    const double DURATION = 2.0;
    const int TOTAL_FRAMES = static_cast<int>(FPS * DURATION);

    fs::path temp_dir = base_output / "remotion_temp";
    if (fs::exists(temp_dir)) fs::remove_all(temp_dir);
    fs::create_directories(temp_dir);

    // Use Composition builder for safety
    auto comp = Composition("comp")
        .size(W, H)
        .fps(FPS)
        .duration(DURATION)
        .layer("bg", [&](LayerBuilder& l) {
            l.solid("bg").color({0, 0, 0, 255}).duration(DURATION).size(W, H);
        })
        .layer("fg", [&](LayerBuilder& l) {
            l.solid("fg").color({10, 10, 10, 255}).start(1.0).duration(1.0).size(W, H)
             .enter().id("tachyon.transition.lightleak.procedural_remotion").duration(1.0).done();
        })
        .build();

    SceneSpec scene;
    scene.compositions.push_back(std::move(comp));

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
        fs::path frame_path = temp_dir / buf;
        result.surface->save_png(frame_path.string());
        
        if (i % 10 == 0) std::cout << "  Frame " << i << "/" << TOTAL_FRAMES << "\n";
    }
    
    fs::path video_path = base_output / "remotion_demo.mp4";
    std::string cmd = "ffmpeg -y -i \"" + temp_dir.string() + "/frame_%03d.png\" -c:v libx264 -preset ultrafast -crf 15 -pix_fmt yuv420p \"" + video_path.string() + "\"";
    
    std::cout << "[FFMPEG] Encoding video...\n";
    std::system(cmd.c_str());
    fs::remove_all(temp_dir);
    
    std::cout << "[DEMO] Render Saved to: " << video_path.string() << "\n";
}

} // namespace tachyon::renderer2d
