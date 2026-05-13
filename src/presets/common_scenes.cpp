#include "tachyon/presets/scene/common.h"
#include "tachyon/transition_registry.h"
#include "tachyon/presets/effects/effect_preset_registry.h"
#include "tachyon/scene/builder.h"

namespace tachyon::presets::scene {

using namespace tachyon;
using namespace tachyon::scene;

SceneSpec build_scene_a() {
    presets::EffectPresetRegistry effects;

    return Composition("main", effects)
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .clear(ColorSpec{25, 25, 204, 255}) // Blue
        .layer(tachyon::presets::text::headline("TESTO SCENA A")
            .font("Default")
            .font_size(120)
            .color(ColorSpec{255, 255, 255, 255})
            .stroke_color(ColorSpec{0, 0, 0, 255})
            .stroke_width(6.0)
            .center()
            .text_box(1920, 1080)
            .position(960, 540)
            .duration(5.0))
        .build_scene();
}

SceneSpec build_scene_b() {
    presets::EffectPresetRegistry effects;

    return Composition("main", effects)
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .clear(ColorSpec{204, 25, 25, 255}) // Red
        .layer(tachyon::presets::text::headline("TESTO SCENA B")
            .font("Default")
            .font_size(120)
            .color(ColorSpec{255, 255, 255, 255})
            .stroke_color(ColorSpec{0, 0, 0, 255})
            .stroke_width(6.0)
            .center()
            .text_box(1920, 1080)
            .position(960, 540)
            .duration(5.0))
        .build_scene();
}

SceneSpec build_text_helpers_scene() {
    presets::EffectPresetRegistry effects;

    return Composition("main", effects)
        .size(1280, 720)
        .duration(5.0)
        .fps(30)
        .clear(ColorSpec{16, 18, 28, 255})
        .layer("title", [](LayerBuilder& l) {
            l.from_spec(tachyon::presets::text::headline("TEXT + 2D HELPERS")
                .font("Default")
                .font_size(92)
                .color(ColorSpec{248, 248, 252, 255})
                .stroke_color(ColorSpec{0, 0, 0, 255})
                .stroke_width(6.0)
                .center()
                .text_box(1280, 720)
                .position(640, 310)
                .build());
        })
        .layer("card", [](LayerBuilder& l) {
            l.solid("card")
             .size(860, 360)
             .fill_color(ColorSpec{242, 158, 66, 255});
        })
        .build_scene();
}

SceneSpec build_video_transition_scene() {
    presets::EffectPresetRegistry effects;

    return Composition("video_transition", effects)
        .size(1920, 1080)
        .duration(3.0)
        .fps(30)
        .layer("clip_a", [](LayerBuilder& l) {
            l.type(static_cast<LayerType>(8))
             .asset_id("C:/Users/pater/Pyt/Tachyon/output/transitions/clip_a.mp4")
             .in(0).out(1.5);
        })
        .layer("clip_b", [](LayerBuilder& l) {
            l.type(static_cast<LayerType>(8))
             .asset_id("C:/Users/pater/Pyt/Tachyon/output/transitions/clip_b.mp4")
             .in(1.5).out(3.0);
        })
        .layer("label", [](LayerBuilder& l) {
            l.from_spec(tachyon::presets::text::headline("VIDEO TRANSITION")
                .font("Default").font_size(120)
                .color({255, 255, 255, 255}).center()
                .position(960, 540).build());
        })
        .build_scene();
}

} // namespace tachyon::presets::scene
