#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) void build_scene(tachyon::SceneSpec& scene) {
    presets::EffectPresetRegistry effects;

    auto main_comp = Composition("main", effects)
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .layer("bg", [](LayerBuilder& l) {
            l.solid("black").color({0,0,0,255}).in(0).out(5.0);
        })
        .layer("text_layer", [](LayerBuilder& l) {
            TextAnimatorSpec v2_anim;
            v2_anim.name = "v2_basic";
            v2_anim.selector.based_on = "characters";
            v2_anim.selector.stagger_delay = 0.1;
            v2_anim.selector.shape = "square";
            v2_anim.selector.offset = 1.0; 
            v2_anim.properties.opacity_value = 0.0;

            l.text()
             .content("TYPEWRITER BASIC")
             .font("SFPro")
             .font_size(120)
             .box(1920, 1080, TextBoxMode::Fixed)
             .align(HorizontalAlign::Center)
             .valign(VerticalAlign::Middle)
             .tracking(1.5f)
             .animator(v2_anim)
             .done()
             .color({255, 255, 255, 255})
             .anchor(960, 540)
             .position(960, 540)
             .in(0).out(5.0);
        })
        .build();

    scene.compositions.push_back(std::move(main_comp));
}
