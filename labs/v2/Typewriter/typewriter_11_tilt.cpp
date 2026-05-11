#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"
#include "tachyon/presets/animation3d/fluent.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" tachyon::SceneSpec build_scene() {
    presets::EffectPresetRegistry effects;

    auto main_comp = Composition("main", effects)
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .layer("bg", [](LayerBuilder& l) {
            l.solid("bg")
             .size(1920.0, 1080.0)
             .position(960.0, 540.0)
             .color({214, 214, 216, 255})
             .in(0).out(5.0);
        })
        .camera3d_layer("cam", [](LayerBuilder& l) {
            l.position3d(0.0, -180.0, 1200.0)
             .camera_poi(0.0, 40.0, 0.0)
             .camera_type("two_node")
             .camera_zoom(48.0);
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light_type("ambient")
             .light_color({255, 255, 255, 255})
             .light_intensity(0.75);
        })
        .light_layer("key", [](LayerBuilder& l) {
            l.light_type("point")
             .position3d(-500.0, 500.0, 400.0)
             .light_color({255, 235, 210, 255})
             .light_intensity(7800000.0);
        })
        .light_layer("rim", [](LayerBuilder& l) {
            l.light_type("point")
             .position3d(600.0, -100.0, -200.0)
             .light_color({180, 120, 255, 255})
             .light_intensity(2500000.0);
        })
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .color({80, 80, 85, 255})
             .size(1000.0, 300.0)
             .anchor(0.0, 0.0)
             .position(960.0, 610.0)
             .rotation3d(-20.0, 0.0, 0.0)
             .position3d(0.0, 0.0, 80.0)
             .material()
                .base_color({80, 80, 85, 255})
                .metallic(0.0)
                .roughness(0.55)
                .emission_color({128, 128, 136, 255})
                .emission_strength(0.18)
                .done()
             .extrude3d(0.08)
             .bevel3d(0.02)
             .in(0).out(5.0);
        })
        .layer("title", [](LayerBuilder& l) {
            l.text()
             .content("TILT")
             .font("default")
             .font_size(220.0)
             .box(1920.0f, 1080.0f)
             .align(HorizontalAlign::Center)
             .valign(VerticalAlign::Middle)
             .color({255, 255, 255, 255})
             .stroke(0, 0, 0, 255, 6.0f)
             .animate(presets::text::kind_typewriter(18.0, "|"))
             .done();

            l.position(960.0, 470.0)
             .in(0).out(5.0);
        })
        .build();

    tachyon::SceneSpec scene;
    scene.compositions.push_back(std::move(main_comp));
    return scene;
}
