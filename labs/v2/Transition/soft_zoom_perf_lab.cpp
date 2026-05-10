#include "tachyon/scene/builder.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) SceneSpec build_scene() {
    SceneBuilder scene;
    
    scene.composition("main", [](CompositionBuilder& comp) {
        comp.size(1920, 1080)
            .fps(60)
            .duration(1.0);

        comp.layer("red", [](LayerBuilder& l) {
            l.solid("red_fill")
             .color({235, 100, 110, 255})
             .in(0.0)
             .out(1.0);
            
            l.exit().id("tachyon.transition.soft_zoom_blur").duration(0.5);
        });

        comp.layer("blue", [](LayerBuilder& l) {
            l.solid("blue_fill")
             .color({100, 160, 230, 255})
             .in(0.5)
             .out(1.0);
            
            l.enter().id("tachyon.transition.soft_zoom_blur").duration(0.5);
        });
    });

    return scene.build();
}
