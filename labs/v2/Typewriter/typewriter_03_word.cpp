#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"
using namespace tachyon;
using namespace tachyon::scene;
extern "C" tachyon::SceneSpec build_scene() {
    presets::EffectPresetRegistry effects;
    auto main_comp = Composition("main", effects).size(1920, 1080).duration(5.0).fps(30)
        .layer("bg", [](LayerBuilder& l) { l.solid("black").color({0,0,0,255}).in(0).out(5.0); })
        .layer("text_layer", [](LayerBuilder& l) {
            l.text().content("TYPEWRITER WORD REVEAL").font("SFPro").font_size(120)
             .box(1920, 1080, TextBoxMode::Fixed)
             .align(HorizontalAlign::Center)
             .valign(VerticalAlign::Middle)
             .animator(presets::text::kind_typewriter_word_cursor(4.0, "|")).done()
             .color({255, 255, 255, 255}).position(960, 540).in(0).out(5.0);
        }).build();
    tachyon::SceneSpec scene;
    scene.compositions.push_back(std::move(main_comp));
    return scene;
}
