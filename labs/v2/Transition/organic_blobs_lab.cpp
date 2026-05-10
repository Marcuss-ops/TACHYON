#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/scene/builder.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) SceneSpec build_scene() {
    SceneBuilder scene;

    scene.composition("organic_blobs_showcase", [](CompositionBuilder &comp) {
        comp.size(1920, 1080).fps(30).duration(4.0);

        // 1. Scene A: White/Ligh background
        comp.layer("scene_a", [](LayerBuilder &l) {
            l.solid("fill_white").color({240, 240, 240, 255}).in(0.0).out(2.5);
        });

        // 2. Scene B: Hard Grey background (per user request)
        comp.layer("scene_b", [](LayerBuilder &l) {
            l.solid("fill_grey").color({60, 60, 60, 255}).in(1.5).out(4.0);
            
            // Organic Blobs transition
            l.enter().id("tachyon.transition.lightleak.organic_blobs").duration(1.5);
        });

        // Label text
        comp.layer("label", [](LayerBuilder &l) {
            l.text("Organic Blobs / Remotion Style C++")
                .font("SFPro", 56)
                .color({255, 255, 255, 255})
                .done()
                .position(100, 800)
                .in(0.0).out(4.0);
        });
    });

    return scene.build();
}
