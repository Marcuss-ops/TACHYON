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
#include <vector>
#include <string>

namespace fs = std::filesystem;

namespace tachyon {
TransitionRegistry create_default_transition_registry();
}

namespace tachyon::renderer2d {

using namespace tachyon::scene;

void render_lookbook_item(const std::string& transition_id, const std::string& name, const TransitionRegistry& trans_reg) {
    std::cout << "\n[LOOKBOOK] Rendering " << name << " (" << transition_id << ")...\n";
    
    fs::path base_output = fs::current_path() / "output" / "lookbook";
    if (!fs::exists(base_output)) {
        fs::create_directories(base_output);
    }

    const int W = 1280;
    const int H = 720;
    const int FPS = 30;
    const double DURATION = 2.0;

    auto comp = Composition("comp")
        .size(W, H)
        .fps(FPS)
        .duration(DURATION)
        .layer("bg", [&](LayerBuilder& l) {
            l.solid("bg").color({20, 20, 25, 255}).duration(DURATION).size(W, H);
        })
        .layer("fg", [&](LayerBuilder& l) {
            l.solid("fg").color({200, 150, 50, 255}).start(0.5).duration(1.5).size(W, H)
             .enter().id(transition_id).duration(1.0).done();
        })
        .build();

    SceneSpec scene;
    scene.compositions.push_back(std::move(comp));

    fs::path video_path = base_output / (name + ".mp4");

    RobustRenderConfig config;
    config.width = W;
    config.height = H;
    config.fps = FPS;
    config.duration = DURATION;
    config.crf = 18;

    render_scene_to_mp4(scene, "comp", video_path.string(), trans_reg, config);
}

void render_transition_lookbook() {
    TransitionRegistry trans_reg = tachyon::create_default_transition_registry();
    
    struct Item { std::string id; std::string name; };
    std::vector<Item> items = {
        {"tachyon.transition.lightleak.remotion", "LightLeak_Remotion"},
        {"tachyon.transition.lightleak.procedural_remotion", "LightLeak_Procedural_Remotion"},
        {"tachyon.transition.light_leak", "LightLeak_Classic"},
        {"tachyon.transition.light_leak_solar", "LightLeak_Solar"},
        {"tachyon.transition.light_leak_sunset", "LightLeak_Sunset"},
        {"tachyon.transition.lightleak.organic_blobs", "LightLeak_Blobs"}
    };

    for (const auto& item : items) {
        render_lookbook_item(item.id, item.name, trans_reg);
    }
}

} // namespace tachyon::renderer2d

bool run_light_leak_transitions_tests() {
    tachyon::renderer2d::render_transition_lookbook();
    return true;
}
