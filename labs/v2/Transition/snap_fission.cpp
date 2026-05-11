#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/scene/builder.h"
using namespace tachyon;
using namespace tachyon::scene;
extern "C" __declspec(dllexport) SceneSpec build_scene() {
    SceneBuilder scene;
    scene.composition("fission_snap", [](CompositionBuilder &comp) {
        comp.size(1280, 720).fps(30).duration(3.5);
        comp.layer("bg", [](LayerBuilder &l) { l.solid("dark").color({20, 20, 22, 255}).in(0.0).out(2.5); });
        comp.layer("fg", [](LayerBuilder &l) {
            l.solid("light").color({220, 220, 225, 255}).in(1.5).out(3.5);
            l.enter().id("tachyon.transition.lightleak.liquid_fission").duration(1.5);
        });
    });
    return scene.build();
}
