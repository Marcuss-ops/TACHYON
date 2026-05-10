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
            l.solid("black").color({0,0,0,255}).in(0).out(5.0);
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
             .intensity(0.32)
             .done();
        })
        .light_layer("key", [](LayerBuilder& l) {
            l.light().type("point")
             .color({255, 242, 225, 255})
             .intensity(1.45)
             .done()
             .transform3d().position(-220.0, 180.0, -220.0).done();
        })
        .layer("text_layer", [](LayerBuilder& l) {
            l.from_spec(tachyon::presets::text::headline("TYPEWRITER BASIC")
                .font("Default")
                .font_size(104)
                .color({255, 255, 255, 255})
                .stroke_color({0, 0, 0, 0})
                .stroke_width(0.0)
                .center()
                .position(960.0, 540.0)
                .text_box(1920, 1080)
                .animate(presets::text::kind_typewriter(20.0, "|"))
                .build());
            l.rotation3d(-12.0, 8.0, 0.0)
             .extrude3d(0.18)
             .bevel3d(0.02)
             .in(0).out(5.0);
            tachyon::presets::animation3d::tilt(l, 12.0, 5.0, 0.65);
        })
        .build();

    tachyon::SceneSpec scene;
    scene.compositions.push_back(std::move(main_comp));
    return scene;
}
