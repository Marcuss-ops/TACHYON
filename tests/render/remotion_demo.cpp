#include "tachyon/jit/tachyon_jit_api.h"
#include "tachyon/scene/builder.h"
#include <iostream>

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif

extern "C" EXPORT const TachyonJitManifest* tachyon_jit_get_manifest() {
    static const TachyonJitManifest manifest = {
        TACHYON_JIT_ABI_VERSION,
        sizeof(TachyonJitManifest),
        "remotion_demo",
        "1.0.0"
    };
    return &manifest;
}

extern "C" EXPORT int tachyon_jit_build_scene(const TachyonHostApi* host, TachyonSceneHandle scene) {
    // Note: We are using the legacy builder path inside the JIT wrapper for now
    // as the full JIT host API is still being expanded.
    return TACHYON_JIT_OK;
}

extern "C" EXPORT tachyon::SceneSpec build_scene() {
    using namespace tachyon;
    using namespace tachyon::scene;
    
    // Remotion Transition Demo - 5 Seconds, 1080p
    
    return Composition("remotion_demo")
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .clear({0, 0, 0, 255})
        .layer("source", [](LayerBuilder& l) {
            l.solid("red").color({200, 0, 0, 255}).duration(5.0);
        })
        .layer("target", [](LayerBuilder& l) {
            l.solid("blue").color({0, 0, 200, 255}).start(0.0).duration(5.0)
             .enter().id("tachyon.transition.lightleak.procedural_remotion").duration(5.0).done();
        })
        .build_scene();
}

