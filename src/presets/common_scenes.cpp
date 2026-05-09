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

SceneSpec build_text_3d_helpers_scene() {
    presets::EffectPresetRegistry effects;

    return Composition("main", effects)
        .size(1280, 720)
        .duration(5.0)
        .fps(30)
        .clear(ColorSpec{16, 18, 28, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
            l.transform3d().position(0.0, 0.0, -520.0).done()
             .camera().poi(0.0, 0.0, 0.0).zoom(40.0).done();
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.9).done();
        })
        .light_layer("key", [](LayerBuilder& l) {
            l.light().type("point").done()
             .transform3d().position(-160.0, 180.0, -220.0).done()
             .light().color({255, 244, 226, 255})
             .intensity(2.6).done();
        })
        .layer("title", [](LayerBuilder& l) {
            l.from_spec(tachyon::presets::text::headline("TEXT + 3D HELPERS")
                .font("Default")
                .font_size(92)
                .color(ColorSpec{248, 248, 252, 255})
                .stroke_color(ColorSpec{0, 0, 0, 255})
                .stroke_width(6.0)
                .center()
                .text_box(1280, 720)
                .position(640, 310)
                .build());
            tachyon::presets::animation3d::tilt(l, 14.0, 8.0, 0.75);
            tachyon::presets::animation3d::parallax(l, 28.0);
            tachyon::presets::animation3d::orbit_y(l, 4.0, 0.0, 0.12);
        })
        .layer("card", [](LayerBuilder& l) {
            l.solid("card")
             .size(860, 360)
             .transform3d().position(0.0, 90.0, 160.0).done()
             .fill_color(ColorSpec{242, 158, 66, 255});
            tachyon::presets::animation3d::tilt(l, 18.0, 10.0, 0.8);
            tachyon::presets::animation3d::parallax(l, 48.0);
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
