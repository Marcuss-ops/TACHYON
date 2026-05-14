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
    
    // Remotion Transition Demo
    // Created to verify the light leak over a black background.
    
    return Composition("remotion_demo")
        .size(1920, 1080)
        .duration(3.0)
        .fps(30)
        .clear({0, 0, 0, 255}) // Black background as requested
        .layer("solid_a", [](LayerBuilder& l) {
            l.solid("a").color({0, 0, 0, 255}).duration(3.0);
        })
        .layer("solid_b", [](LayerBuilder& l) {
            l.solid("b").color({0, 0, 0, 255}).start(0.0).duration(3.0)
             .enter().id("tachyon.transition.lightleak.remotion").duration(3.0).done();
        })
        .build_scene();
}
