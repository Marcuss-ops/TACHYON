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
            anim.name = "stagger_glow";
            anim.selector.type = "all";
            anim.selector.based_on = "characters";
            anim.selector.stagger_delay = 0.12;
            
            anim.properties.opacity_keyframes.push_back({0.0, 0.0});
            anim.properties.opacity_keyframes.push_back({0.6, 1.0});
            
            // Subtle color shift from dark gray to bright white
            anim.properties.fill_color_keyframes.push_back({0.0, {100, 100, 100, 255}});
            anim.properties.fill_color_keyframes.push_back({0.6, {255, 255, 255, 255}});

            l.text().content("STAGGERED GLOW REVEAL").font("SFPro").font_size(105)
             .box(1920, 1080, TextBoxMode::Fixed)
             .align(HorizontalAlign::Center)
             .valign(VerticalAlign::Middle)
             .tracking(-3.5f)
             .animator(anim).done()
             .color({255, 255, 255, 255}).position(960, 540).in(0).out(5.0);
        }).build();

    tachyon::SceneSpec scene;
    scene.compositions.push_back(std::move(main_comp));
    return scene;
}
