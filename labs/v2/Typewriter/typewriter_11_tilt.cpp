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
            l.solid("bg").color({35, 35, 41, 255}).in(0).out(5.0);
        })
        .camera3d_layer("cam", [](LayerBuilder& l) {
            l.transform3d().position(0.0, -120.0, -980.0).done()
             .camera().type("two_node")
             .poi(0.0, 70.0, 0.0)
             .zoom(48.0)
             .done();
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.38)
             .done();
        })
        .light_layer("key", [](LayerBuilder& l) {
            l.light().type("point")
             .color({255, 245, 230, 255})
             .intensity(1.65)
             .done()
             .transform3d().position(-260.0, 200.0, -180.0).done();
        })
        .light_layer("rim", [](LayerBuilder& l) {
            l.light().type("point")
             .color({190, 210, 255, 255})
             .intensity(0.85)
             .done()
             .transform3d().position(220.0, 60.0, -220.0).done();
        })
        .layer("plate", [](LayerBuilder& l) {
            l.solid("plate")
             .color({62, 66, 80, 255})
             .size(1500.0, 560.0)
             .anchor(0.0, 0.0)
             .position(960.0, 540.0)
             .rotation3d(-10.0, 18.0, 0.0)
             .position3d(0.0, 0.0, 40.0)
             .extrude3d(0.08)
             .bevel3d(0.02)
             .in(0).out(5.0);
        })
        .layer("title", [](LayerBuilder& l) {
            l.from_spec(tachyon::presets::text::headline("TILT")
                .font("Default")
                .font_size(120)
                .color({255, 255, 255, 255})
                .stroke_color({0, 0, 0, 0})
                .stroke_width(0.0)
                .center()
                .position(960.0, 540.0)
                .text_box(1920, 1080)
                .animate(presets::text::kind_typewriter(18.0, "|"))
                .build());

            l.rotation3d(-14.0, 18.0, 0.0)
             .position3d(0.0, 0.0, 92.0)
             .extrude3d(0.40)
             .bevel3d(0.06)
             .scale3d(1.35, 1.35, 1.0)
             .in(0).out(5.0);
            tachyon::presets::animation3d::tilt(l, 18.0, 5.0, 0.75);
        })
        .build();

    tachyon::SceneSpec scene;
    scene.compositions.push_back(std::move(main_comp));
    return scene;
}
