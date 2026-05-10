#include "tachyon/scene/builder.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) tachyon::SceneSpec build_scene() {
    SceneBuilder scene;
    
    scene.composition("slide_test", [](CompositionBuilder& comp) {
        comp.size(1920, 1080)
            .fps(30)
            .duration(4.0);
        
        // Background color (just in case)
        comp.layer("bg", [](LayerBuilder& l) {
            l.solid("fill")
             .color({30, 30, 30, 255})
             .in(0.0)
             .out(4.0);
        });

        // Scena A (Red) from 0.0s to 2.5s
        comp.layer("scena_a", [](LayerBuilder& l) {
            l.solid("fill")
             .color({235, 100, 110, 255}) // Red
             .in(0.0)
             .out(2.5);
            
            // Transition out for 1 second
            l.exit().id("tachyon.transition.slide").duration(1.0);
        });

        // Scena B (Blue) from 1.5s to 4.0s (1 second overlap)
        comp.layer("scena_b", [](LayerBuilder& l) {
            l.solid("fill")
             .color({100, 160, 230, 255}) // Blue
             .in(1.5)
             .out(4.0);
            
            // Transition in for 1 second
            l.enter().id("tachyon.transition.slide").duration(1.0);
        });
    });

    return scene.build();
}
