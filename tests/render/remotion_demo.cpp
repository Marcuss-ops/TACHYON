#include "tachyon/scene/builder.h"
#include <iostream>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

extern "C" EXPORT tachyon::SceneSpec build_scene() {
    using namespace tachyon;
    using namespace tachyon::scene;
    
    // Procedural Remotion Transition Demo
    // Created to verify the new transition over a black background.
    
    return Composition("remotion_demo")
        .size(1920, 1080)
        .duration(2.0)
        .fps(30)
        .clear({0, 0, 0, 255}) // Black background as requested
        .layer("solid_a", [](LayerBuilder& l) {
            l.solid("a").color({20, 20, 20, 255}).duration(2.0);
        })
        .layer("solid_b", [](LayerBuilder& l) {
            l.solid("b").color({30, 30, 30, 255}).start(1.0).duration(1.0)
             .enter().id("tachyon.transition.lightleak.procedural_remotion").duration(1.0).done();
        })
        .build_scene();
}
