#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/scene/builder.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) SceneSpec build_scene() {
    SceneBuilder scene;

    scene.composition("organic_blobs_showcase", [](CompositionBuilder &comp) {
        comp.size(1920, 1080).fps(30).duration(4.0);

        // 1. Scene A: Hard Gray Background
        comp.layer("scene_a", [](LayerBuilder &l) {
            l.solid("fill_gray_a").color({40, 40, 40, 255}).in(0.0).out(4.0);
        });

        // 2. Scene B: Transition Overlay
        comp.layer("scene_b", [](LayerBuilder &l) {
            l.solid("fill_gray_b").color({40, 40, 40, 255}).in(0.0).out(4.0);
            
            // Route to rewritten dynamic single amber blob kernel covering full 4s duration
            l.enter().id("tachyon.transition.light_leak_ghost").duration(4.0);
        });
    });

    return scene.build();
}
