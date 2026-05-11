#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/scene/builder.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) SceneSpec build_scene() {
    SceneBuilder scene;

    // Composition Showcase: Cinematic Amber (Dynamic Hue Fluid)
    scene.composition("premium_cinematic_amber", [&](CompositionBuilder &comp) {
        comp.size(1280, 720).fps(30).duration(5.0);
        comp.clear({25, 25, 27, 255});
        comp.layer("base_field", [](LayerBuilder &l) {
            l.solid("solid_a").color({25, 25, 27, 255}).size(1280, 720).position(640, 360).in(0.0).out(5.0);
        });
        comp.layer("overlay_field", [](LayerBuilder &l) {
            l.solid("solid_b").color({25, 25, 27, 255}).size(1280, 720).position(640, 360).in(1.5).out(5.0);
            l.enter().id("tachyon.transition.lightleak.cinematic_amber").duration(2.5); // Elegantly slow
        });
    });

    // Composition 2: Liquid Fission
    scene.composition("premium_liquid_fission", [&](CompositionBuilder &comp) {
        comp.size(1280, 720).fps(30).duration(5.0);
        comp.clear({25, 25, 27, 255});
        comp.layer("base_field", [](LayerBuilder &l) {
            l.solid("solid_a").color({25, 25, 27, 255}).size(1280, 720).position(640, 360).in(0.0).out(5.0);
        });
        comp.layer("overlay_field", [](LayerBuilder &l) {
            l.solid("solid_b").color({25, 25, 27, 255}).size(1280, 720).position(640, 360).in(1.5).out(5.0);
            l.enter().id("tachyon.transition.lightleak.liquid_fission").duration(2.0);
        });
    });

    // Composition 1: Lava Flow
    scene.composition("premium_lava_flow", [&](CompositionBuilder &comp) {
        comp.size(1280, 720).fps(30).duration(5.0);
        comp.clear({25, 25, 27, 255});
        comp.layer("base_field", [](LayerBuilder &l) {
            l.solid("solid_a").color({25, 25, 27, 255}).size(1280, 720).position(640, 360).in(0.0).out(5.0);
        });
        comp.layer("overlay_field", [](LayerBuilder &l) {
            l.solid("solid_b").color({25, 25, 27, 255}).size(1280, 720).position(640, 360).in(1.5).out(5.0);
            l.enter().id("tachyon.transition.lightleak.lava_flow").duration(2.0);
        });
    });

    // Composition 3: Cosmic Swirl
    scene.composition("premium_cosmic_swirl", [&](CompositionBuilder &comp) {
        comp.size(1280, 720).fps(30).duration(5.0);
        comp.clear({25, 25, 27, 255}); // Native fill-backplate ensuring absolute solid uniformity
        comp.layer("base_field", [](LayerBuilder &l) {
            l.solid("solid_a").color({25, 25, 27, 255}).size(1280, 720).position(640, 360).in(0.0).out(5.0);
        });
        comp.layer("overlay_field", [](LayerBuilder &l) {
            l.solid("solid_b").color({25, 25, 27, 255}).size(1280, 720).position(640, 360).in(1.5).out(5.0);
            l.enter().id("tachyon.transition.lightleak.cosmic_swirl").duration(2.0);
        });
    });

    return scene.build();
}
