#include "tachyon/scene/builder.h"
#include "tachyon/presets/animation3d/fluent.h"
#include "tachyon/presets/text/fluent.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" tachyon::SceneSpec build_scene() {
    presets::EffectPresetRegistry effects;

    auto main_comp = Composition("main", effects)
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .clear({213, 213, 213, 255})
        .camera3d_layer("cam", [](LayerBuilder& l) {
            l.transform3d().position(-180.0, -24.0, -680.0).done()
             .camera().poi(0.0, 0.0, 0.0).zoom(58.0).done();
        })
        .light_layer("ambient", [](LayerBuilder& l) {
            l.light().type("ambient")
             .color({255, 255, 255, 255})
             .intensity(0.42)
             .done();
        })
        .light_layer("key", [](LayerBuilder& l) {
            l.light().type("point")
             .color({255, 248, 235, 255})
             .intensity(3.2)
             .done()
             .transform3d().position(-380.0, 280.0, -360.0).done();
        })
        .layer("title", [](LayerBuilder& l) {
            l.text().content("TILT")
             .font("SFPro")
             .font_size(380)
             .box(1920, 1080)
             .align(HorizontalAlign::Center)
             .valign(VerticalAlign::Middle)
             .tracking(0.0f)
             .done();

            l.position(960.0, 540.0);
            l.rotation3d(-10.0, -28.0, 0.0)
             .position3d(0.0, 0.0, 0.0)
             .scale3d(1.65, 1.65, 1.0)
             .extrude3d(1.25)
             .bevel3d(0.16);
            l.material()
             .base_color({248, 248, 250, 255})
             .roughness(0.22)
             .metallic(0.0)
             .done();
        })
        .build();

    SceneSpec scene;
    scene.compositions.push_back(std::move(main_comp));
    return scene;
}
