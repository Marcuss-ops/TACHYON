#include "tachyon/scene/builder.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <iostream>

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) SceneSpec build_scene() {
    std::cerr << "!!! build_scene start !!!" << std::endl;
    SceneBuilder scene;
    scene.composition("test", [](CompositionBuilder& comp) {
        comp.size(1920, 1080).fps(30).duration(2.0);
        comp.layer("bg", [](LayerBuilder& l) {
            l.solid("fill").color({255, 0, 0, 255}).in(0.0).out(2.0);
            l.exit().id("tachyon.transition.slide").duration(1.0);
        });
        comp.layer("fg", [](LayerBuilder& l) {
            l.solid("fill").color({0, 255, 0, 255}).in(1.0).out(2.0);
            l.enter().id("tachyon.transition.slide").duration(1.0);
        });
    });
    std::cerr << "!!! build_scene before scene.build() !!!" << std::endl;
    SceneSpec result = scene.build();
    std::cerr << "!!! build_scene after scene.build() !!!" << std::endl;
    return result;
}
