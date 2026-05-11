#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/scene/builder.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) SceneSpec build_scene() {
    SceneBuilder scene;

    scene.composition("isolated_light_leak_test", [](CompositionBuilder &comp) {
        comp.size(1920, 1080).fps(30).duration(3.0);

        // Scene A: Absolute black baseline
        comp.layer("scene_a", [](LayerBuilder &l) {
            l.solid("black_base").color({0, 0, 0, 255}).in(0.0).out(3.0);
        });

        // Scene B: Overlap layer with ONLY the transition active over it, but with a recognizable base color (Green) to see if it mixes
        comp.layer("scene_b", [](LayerBuilder &l) {
            l.solid("green_test").color({0, 255, 0, 255}).in(0.5).out(3.0);
            
            // Apply Light Leak Ghost Transition (which binds to our Blobs code)
            l.enter().id("tachyon.transition.light_leak_ghost").duration(2.0);
        });
    });

    return scene.build();
}
