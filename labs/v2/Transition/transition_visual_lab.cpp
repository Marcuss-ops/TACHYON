#include "tachyon/scene/builder.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) SceneSpec build_scene() {
    SceneBuilder scene;
    
    scene.composition("main", [](CompositionBuilder& comp) {
        comp.size(1920, 1080)
            .fps(60)
            .duration(4.8);

        // --- SCENE A ---
        comp.layer("red", [](LayerBuilder& l) {
            l.solid("red_fill")
             .color({235, 100, 110, 255})
             .in(0.0)
             .out(2.8);
            
            l.exit().id("tachyon.transition.crossfade").duration(0.8);
        });

        comp.layer("text_a", [](LayerBuilder& l) {
            l.text("Scena A")
             .font("Impact", 200)
             .color(255, 255, 255, 255)
             .centerText()
             .done()
             .in(0.0)
             .out(2.8);
            
            l.exit().id("tachyon.transition.crossfade").duration(0.8);
        });

        // --- SCENE B ---
        comp.layer("blue", [](LayerBuilder& l) {
            l.solid("blue_fill")
             .color({100, 160, 230, 255})
             .in(2.0)
             .out(4.8);
            
            l.enter().id("tachyon.transition.crossfade").duration(0.8);
        });

        comp.layer("text_b", [](LayerBuilder& l) {
            l.text("Scena B")
             .font("Courier New", 200)
             .color(255, 255, 255, 255)
             .centerText()
             .done()
             .in(2.0)
             .out(4.8);
            
            l.enter().id("tachyon.transition.crossfade").duration(0.8);
        });
    });

    return scene.build();
}
