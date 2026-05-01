#pragma once

#include "tachyon/scene/builder.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include "tachyon/core/spec/schema/objects/composition_spec.h"
#include "tachyon/core/spec/schema/objects/layer_spec.h"
#include "tachyon/core/spec/schema/animation/text_animator_spec.h"
#include "tachyon/core/types/colors.h"
#include "tachyon/core/animation/easing.h"

namespace tachyon::test::fixtures {

inline SceneSpec make_canonical_scene() {
    using namespace scene;

    return Scene()
        .project("proj_001", "Canonical Scene")
        .composition("main", [](auto& c) {
            c.size(1920, 1080)
             .fps(30)
             .duration(120.0)
             .background({"assets/background.png"})
             .layer("title", [](LayerBuilder& l) {
                 l.type("text")
                  .text("Title")
                  .in(0.0)
                  .out(120.0)
                  .opacity(1.0);
             });
         })
        .build();
}

inline SceneSpec make_scene_graph_minimal() {
    using namespace scene;

    return Scene()
        .project("proj_scene_graph", "Scene Graph Minimal")
        .composition("main", [](auto& c) {
            c.size(1920, 1080)
             .fps(30)
             .duration(10.0)
             .layer("parent", [](LayerBuilder& l) {
                 l.type("null")
                  .name("Parent")
                  .in(0.0)
                  .out(10.0)
                  .opacity(0.5)
                  .position(100.0, 50.0);
             })
             .layer("child", [](LayerBuilder& l) {
                 l.type("solid")
                  .name("Child")
                  .in(0.0)
                  .out(10.0)
                  .opacity(0.75)
                  .position(10.0, 5.0);
             });
         })
        .build();
}

inline SceneSpec make_template_scene() {
    using namespace scene;

    return Scene()
        .project("proj_template", "Template Test")
        .composition("main", [](auto& c) {
            c.size(1920, 1080)
             .fps(30)
             .duration(10.0)
             .layer("text_layer", [](LayerBuilder& l) {
                 l.type("text")
                  .name("Text Layer")
                  .text("Hello Intro / {{score}}")
                  .in(0.0)
                  .out(10.0)
                  .opacity(1.0);
             });
         })
        .build();
}

inline SceneSpec make_text_preview_scene() {
    using namespace scene;
    using tachyon::ColorSpec;

    auto make_animator = [](double stagger_delay, double reveal_end) {
        TextAnimatorSpec anim;
        anim.selector.based_on = "characters_excluding_spaces";
        anim.selector.stagger_mode = "character";
        anim.selector.stagger_delay = stagger_delay;

        anim.properties.opacity_keyframes = {
            anim::keyframes({{0.0, 0.0}, {reveal_end, 1.0}})
        };
        anim.properties.position_offset_keyframes = {
            anim::keyframes({{0.0, 46.0}, {reveal_end, 0.0}})
        };
        anim.properties.reveal_keyframes = {
            anim::keyframes({{0.0, 0.0}, {reveal_end, 1.0}})
        };
        return anim;
    };

    auto make_animator_full = [](double stagger_delay, double reveal_end) {
        TextAnimatorSpec anim;
        anim.selector.based_on = "characters";
        anim.selector.stagger_mode = "character";
        anim.selector.stagger_delay = stagger_delay;

        anim.properties.opacity_keyframes = {
            anim::keyframes({{0.0, 0.0}, {reveal_end, 1.0}})
        };
        anim.properties.position_offset_keyframes = {
            anim::keyframes({{0.0, 22.0}, {reveal_end, 0.0}})
        };
        anim.properties.tracking_amount_keyframes = {
            anim::keyframes({{0.0, 18.0}, {reveal_end, 0.0}})
        };
        anim.properties.scale_keyframes = {
            anim::keyframes({{0.0, 0.96}, {reveal_end, 1.0}})
        };
        anim.properties.reveal_keyframes = {
            anim::keyframes({{0.0, 0.0}, {reveal_end, 1.0}})
        };
        return anim;
    };

    return Scene()
        .project("text_preview", "Text Preview")
        .composition("main", [](auto& c) {
            c.size(1920, 1080)
             .fps(15)
             .duration(5.0)
             .layer("background", [](LayerBuilder& l) {
                 l.type("solid")
                  .name("Background")
                  .size(1920, 1080)
                  .in(0.0)
                  .out(5.0)
                  .fill_color(ColorSpec{10, 14, 24, 255});
             })
             .layer("headline", [](LayerBuilder& l) {
                 l.type("text")
                  .name("Headline")
                  .text("TACHYON")
                  .size(1920, 220)
                  .font_size(136)
                  .fill_color(ColorSpec{245, 248, 255, 255})
                  .position(0.0, 150.0)
                  .in(0.0)
                  .out(5.0)
                  .alignment("center")
                  .text_animator(make_animator(0.025, 0.85));
             })
             .layer("subhead", [](LayerBuilder& l) {
                 l.type("text")
                  .name("Subhead")
                  .text("FRASI IMPORTANTI")
                  .size(1920, 180)
                  .font_size(62)
                  .fill_color(ColorSpec{190, 198, 216, 255})
                  .position(0.0, 320.0)
                  .in(0.0)
                  .out(5.0)
                  .alignment("center")
                  .text_animator(make_animator(0.035, 0.75));
             })
             .layer("date", [](LayerBuilder& l) {
                 l.type("text")
                  .name("Date")
                  .text("27 APRILE 2026")
                  .size(1920, 180)
                  .font_size(74)
                  .fill_color(ColorSpec{255, 213, 122, 255})
                  .position(0.0, 520.0)
                  .in(0.0)
                  .out(5.0)
                  .alignment("center")
                  .text_animator(make_animator_full(0.02, 0.6));
             })
             .layer("numbers", [](LayerBuilder& l) {
                 l.type("text")
                  .name("Numbers")
                  .text("001 | 128 | 2048")
                  .size(1920, 180)
                  .font_size(56)
                  .fill_color(ColorSpec{101, 225, 255, 255})
                  .position(0.0, 700.0)
                  .in(0.0)
                  .out(5.0)
                  .alignment("center")
                  .text_animator(make_animator_full(0.02, 0.55));
             });
         })
        .build();
}

} // namespace tachyon::test::fixtures
