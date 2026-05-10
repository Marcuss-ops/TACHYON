#include "tachyon/scene/builder.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) SceneSpec build_scene() {
    SceneBuilder scene;
    
    scene.composition("main", [](CompositionBuilder& comp) {
        comp.size(1920, 1080)
            .fps(60)
            .duration(2.0);

        // Background
        comp.layer("red_solid", [](LayerBuilder& l) {
            l.solid("background")
             .color({235, 100, 110, 255})
             .in(0.0)
             .out(2.0);
        });

        // Text
        comp.layer("text_a", [](LayerBuilder& l) {
            l.text("Scena A")
             .font("Impact", 200)
             .color(255, 255, 255, 255)
             .centerText()
             .done()
             .in(0.0)
             .out(2.0);
        });
    });

    return scene.build();
}
