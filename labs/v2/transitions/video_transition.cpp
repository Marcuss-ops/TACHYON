#include "tachyon/scene/builder.h"
#include "tachyon/core/ids/builtin_ids.h"
#include <iostream>

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) void build_scene(tachyon::SceneSpec& scene) {
    presets::EffectPresetRegistry effects;
    
    auto comp = Composition("video_transition", effects)
        .size(1920, 1080)
        .duration(3.0)
        .fps(30);

    // CLIP A: 0.0s -> 2.0s
    comp.layer("clip_a", [&](LayerBuilder& l) {
        l.type(static_cast<LayerType>(8))
         .asset_id("C:\\Users\\pater\\Pyt\\Tachyon\\output\\transitions\\clip_a.mp4")
         .in(0.0).out(2.0);
    });

    // CLIP B: 1.0s -> 3.0s (Transition starts at 1.0s)
    comp.layer("clip_b", [&](LayerBuilder& l) {
        l.type(static_cast<LayerType>(8))
         .asset_id("C:\\Users\\pater\\Pyt\\Tachyon\\output\\transitions\\clip_b.mp4")
         .in(1.0).out(3.0)
         .size(1920, 1080)
         .position(960, 540)
         .enter().id("tachyon.transition.smooth_wipe").duration(1.0).done();
    });

    // Label for clarity
    comp.layer("label", [&](LayerBuilder& l) {
        l.text().content("VIDEO TRANSITION").font("SFPro").font_size(60).box(1920, 100).align(HorizontalAlign::Center).done()
         .position(960, 1020).in(0).out(3.0);
    });

    auto built_comp = comp.build();
    scene.compositions.push_back(std::move(built_comp));
}
