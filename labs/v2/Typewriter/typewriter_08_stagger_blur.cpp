#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" tachyon::SceneSpec build_scene() {
    presets::EffectPresetRegistry effects;

    auto main_comp = Composition("main", effects).size(1920, 1080).duration(5.0).fps(30)
        .layer("bg", [](LayerBuilder& l) { l.solid("black").color({0, 0, 0, 255}).in(0).out(5.0); })
        .layer("text_layer", [](LayerBuilder& l) {
            TextAnimatorSpec anim;
            anim.name = "kinetic_rise";
            anim.selector.type = "all";
            anim.selector.based_on = "characters";
            anim.selector.stagger_delay = 0.1;
            
            anim.properties.opacity_keyframes.push_back({0.0, 0.0});
            anim.properties.opacity_keyframes.push_back({0.5, 1.0});
            
            anim.properties.position_offset_keyframes.push_back({0.0, {0.0, 60.0}});
            anim.properties.position_offset_keyframes.push_back({0.5, {0.0, -5.0}}); // Overshoot
            anim.properties.position_offset_keyframes.push_back({0.7, {0.0, 0.0}});

            l.text().content("KINETIC RISE UP").font("SFPro").font_size(115)
             .box(1920, 1080, TextBoxMode::Fixed)
             .align(HorizontalAlign::Center)
             .valign(VerticalAlign::Middle)
             .tracking(-3.0f)
             .animator(anim).done()
             .color({255, 255, 255, 255}).position(960, 540).in(0).out(5.0);
        }).build();

    tachyon::SceneSpec scene;
    scene.compositions.push_back(std::move(main_comp));
    return scene;
}
