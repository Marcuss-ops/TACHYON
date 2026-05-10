#include "tachyon/scene/builder.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/backgrounds.hpp"

using namespace tachyon;
using namespace tachyon::scene;
using namespace tachyon::presets;

// This script expects a PALETTE define to be passed during JIT compilation,
// or it defaults to premium_dark.
#ifndef PALETTE_NAME
#define PALETTE_NAME "premium_dark"
#endif

extern "C" SceneSpec build_scene() {
    EffectPresetRegistry effects;

    BackgroundParams p;
    p.kind = "tachyon.background.kind.aura";
    p.palette = PALETTE_NAME;
    p.w = 1920;
    p.h = 1080;
    p.in_point = 0;
    p.out_point = 5.0; // 5 seconds as requested for consistency
    p.speed = 1.0f;

    auto main_comp = Composition("main", effects)
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .layer("bg", [p](LayerBuilder& l) {
            l.background(build_background(p));
        })
        .build();

    SceneSpec scene;
    scene.compositions.push_back(std::move(main_comp));
    return scene;
}

