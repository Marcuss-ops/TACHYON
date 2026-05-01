#ifndef TESTS_FIXTURES_MAKE_SCENES_H
#define TESTS_FIXTURES_MAKE_SCENES_H

#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"
#include <vector>

namespace test_fixtures {

using namespace tachyon::scene;
using namespace tachyon::presets::text;

SceneSpec make_text_preview_scene() {
    return Composition("main")
        .size(1920, 1080)
        .duration(5.0)
        .fps(15)
        .layer("background", [](LayerBuilder& l) {
            l.type("solid")
             .size(1920, 1080)
             .fill_color({10, 14, 24, 255})
             .start_time(0)
             .in_point(0)
             .out_point(5)
             .enabled(true)
             .visible(true);
        })
        .layer("headline", [](LayerBuilder& l) {
            l.type("text")
             .size(1920, 220)
             .text_content("TACHYON")
             .font_size(136)
             .fill_color({245, 248, 255, 255})
             .alignment("center")
             .start_time(0)
             .in_point(0)
             .out_point(5)
             .enabled(true)
             .visible(true)
             .transform_position(0, 150)
             .text_animator(AnimatorBuilder()
                 .based_on("characters_excluding_spaces")
                 .stagger_mode("character")
                 .stagger_delay(0.025)
                 .opacity_keyframe(0, 0)
                 .opacity_keyframe(0.85, 1)
                 .position_offset_keyframe(0, {0, 46})
                 .position_offset_keyframe(0.85, {0, 0})
                 .reveal_keyframe(0, 0)
                 .reveal_keyframe(0.85, 1)
                 .build());
        })
        .layer("subhead", [](LayerBuilder& l) {
            l.type("text")
             .size(1920, 180)
             .text_content("FRASI IMPORTANTI")
             .font_size(62)
             .fill_color({190, 198, 216, 255})
             .alignment("center")
             .start_time(0)
             .in_point(0)
             .out_point(5)
             .enabled(true)
             .visible(true)
             .transform_position(0, 320)
             .text_animator(AnimatorBuilder()
                 .based_on("characters_excluding_spaces")
                 .stagger_mode("character")
                 .stagger_delay(0.035)
                 .opacity_keyframe(0, 0)
                 .opacity_keyframe(0.75, 1)
                 .position_offset_keyframe(0, {0, 30})
                 .position_offset_keyframe(0.75, {0, 0})
                 .reveal_keyframe(0, 0)
                 .reveal_keyframe(0.75, 1)
                 .build());
        })
        .layer("date", [](LayerBuilder& l) {
            l.type("text")
             .size(1920, 180)
             .text_content("27 APRILE 2026")
             .font_size(74)
             .fill_color({255, 213, 122, 255})
             .alignment("center")
             .start_time(0)
             .in_point(0)
             .out_point(5)
             .enabled(true)
             .visible(true)
             .transform_position(0, 520)
             .text_animator(AnimatorBuilder()
                 .based_on("characters")
                 .stagger_mode("character")
                 .stagger_delay(0.02)
                 .opacity_keyframe(0, 0)
                 .opacity_keyframe(0.6, 1)
                 .position_offset_keyframe(0, {0, 22})
                 .position_offset_keyframe(0.6, {0, 0})
                 .tracking_amount_keyframe(0, 18)
                 .tracking_amount_keyframe(0.6, 0)
                 .scale_keyframe(0, 0.96)
                 .scale_keyframe(0.6, 1)
                 .reveal_keyframe(0, 0)
                 .reveal_keyframe(0.6, 1)
                 .build());
        })
        .layer("numbers", [](LayerBuilder& l) {
            l.type("text")
             .size(1920, 180)
             .text_content("001 | 128 | 2048")
             .font_size(56)
             .fill_color({101, 225, 255, 255})
             .alignment("center")
             .start_time(0)
             .in_point(0)
             .out_point(5)
             .enabled(true)
             .visible(true)
             .transform_position(0, 700)
             .text_animator(AnimatorBuilder()
                 .based_on("characters")
                 .stagger_mode("character")
                 .stagger_delay(0.02)
                 .opacity_keyframe(0, 0)
                 .opacity_keyframe(0.55, 1)
                 .position_offset_keyframe(0, {0, 18})
                 .position_offset_keyframe(0.55, {0, 0})
                 .tracking_amount_keyframe(0, 18)
                 .tracking_amount_keyframe(0.55, 0)
                 .scale_keyframe(0, 0.96)
                 .scale_keyframe(0.55, 1)
                 .reveal_keyframe(0, 0)
                 .reveal_keyframe(0.55, 1)
                 .build());
        })
        .build_scene();
}

SceneSpec make_template_scene() {
    return Composition("main")
        .size(1920, 1080)
        .duration(10.0)
        .fps(30)
        .layer("text_layer", [](LayerBuilder& l) {
            l.type("text")
             .name("Text Layer")
             .text_content("Hello Intro / {{score}}")
             .start_time(0)
             .in_point(0)
             .out_point(10)
             .opacity(1.0)
             .enabled(true)
             .visible(true);
        })
        .build_scene();
}

SceneSpec make_scene_graph_minimal_scene() {
    return Composition("main")
        .size(1920, 1080)
        .duration(10.0)
        .fps(30)
        .layer("parent", [](LayerBuilder& l) {
            l.type("null")
             .name("Parent")
             .start_time(0)
             .in_point(0)
             .out_point(10)
             .opacity(0.5)
             .time_remap(0.25)
             .transform_position(100, 50)
             .transform_scale(2.0, 2.0)
             .enabled(true)
             .visible(true);
        })
        .layer("child", [](LayerBuilder& l) {
            l.type("solid")
             .name("Child")
             .start_time(0)
             .in_point(0)
             .out_point(10)
             .opacity(0.75)
             .time_remap(1.5)
             .parent("parent")
             .transform_position(10, 5)
             .transform_scale(0.5, 0.5)
             .enabled(true)
             .visible(true);
        })
        .build_scene();
}

SceneSpec make_canonical_scene() {
    return Composition("main")
        .size(1920, 1080)
        .duration(120.0)
        .fps(30)
        .asset("logo", "image", "assets/logo.png")
        .asset("background", "image", "assets/background.png")
        .layer("title", [](LayerBuilder& l) {
            l.type("text")
             .name("Title")
             .start_time(0)
             .in_point(0)
             .out_point(120)
             .opacity(1.0)
             .enabled(true)
             .visible(true);
        })
        .build_scene();
}

} // namespace test_fixtures

#endif // TESTS_FIXTURES_MAKE_SCENES_H
