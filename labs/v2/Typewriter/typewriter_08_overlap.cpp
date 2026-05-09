#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
using namespace tachyon;
using namespace tachyon::scene;
extern "C" tachyon::SceneSpec build_scene() {
    presets::EffectPresetRegistry effects;
    auto main_comp = Composition("main", effects).size(1920, 1080).duration(5.0).fps(30)
        .layer("bg", [](LayerBuilder& l) { l.solid("black").color({0,0,0,255}).in(0).out(5.0); })
        .layer("text_layer", [](LayerBuilder& l) {
            TextAnimatorSpec v2_anim;
            v2_anim.name = "v2_overlap";
            v2_anim.selector.based_on = "characters";
            v2_anim.selector.stagger_delay = 0.04;
            v2_anim.selector.shape = "ramp_up";
            v2_anim.selector.offset = 1.0; 
            v2_anim.properties.opacity_value = 0.0;
            l.text().content("TYPEWRITER OVERLAP").font("SFPro").font_size(120)
             .box(1920, 1080, TextBoxMode::Fixed)
             .align(HorizontalAlign::Center)
             .valign(VerticalAlign::Middle)
             .animator(v2_anim).done()
             .color({255, 255, 255, 255}).position(960, 540).in(0).out(5.0);
        }).build();
    tachyon::SceneSpec scene;
    scene.compositions.push_back(std::move(main_comp));
    return scene;
}
