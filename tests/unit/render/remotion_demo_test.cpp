#include "render_test_utils.h"
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

namespace fs = std::filesystem;

namespace tachyon {
TransitionRegistry create_default_transition_registry();
}

namespace tachyon::renderer2d {

using namespace tachyon::scene;

void render_remotion_demo() {
    std::cout << "[DEMO] Starting Robust Remotion Demo...\n";
    
    fs::path base_output = fs::current_path() / "output" / "transitions";
    if (!fs::exists(base_output)) {
        fs::create_directories(base_output);
    }

    TransitionRegistry trans_reg = tachyon::create_default_transition_registry();
    
    const int W = 1920;
    const int H = 1080;
    const int FPS = 30;
    const double DURATION = 2.0;

    auto comp = Composition("comp")
        .size(W, H)
        .fps(FPS)
        .duration(DURATION)
        .layer("bg", [&](LayerBuilder& l) {
            l.solid("bg").color({0, 0, 0, 255}).duration(DURATION).size(W, H);
        })
        .layer("fg", [&](LayerBuilder& l) {
            l.solid("fg").color({0, 0, 0, 255}).start(0.0).duration(DURATION).size(W, H)
             .enter().id("tachyon.transition.lightleak.remotion").duration(DURATION).done();
        })
        .build();

    SceneSpec scene;
    scene.compositions.push_back(std::move(comp));

    fs::path video_path = base_output / "remotion_demo.mp4";

    RobustRenderConfig config;
    config.width = W;
    config.height = H;
    config.fps = FPS;
    config.duration = DURATION;
    config.crf = 18;
    config.preset = "medium";

    render_scene_to_mp4(scene, "comp", video_path.string(), trans_reg, config);
}

} // namespace tachyon::renderer2d
