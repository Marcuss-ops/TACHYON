#include "test_utils.h"
#include "image_comparison.h"
#include "tachyon/scene/builder.h"
#include <vector>
#include <iostream>

namespace tachyon::test {

using namespace tachyon::scene;

bool run_golden_sequence_tests() {
    std::cout << "[Golden] Running Extended Visual Regression Sequence Tests...\n";
    bool all_ok = true;

    // 1. solid_basic
    {
        auto scene = Composition("solid_basic")
            .size(256, 256)
            .duration(1.0)
            .fps(30)
            .clear(ColorSpec{20, 40, 60, 255}) // dark blue background
            .layer("green_solid", [](LayerBuilder& l) {
                l.solid("green").color({40, 200, 80, 255})
                 .position(50, 50).size(150, 150).duration(1.0);
            })
            .build_scene();

        all_ok = verify_golden_sequence("solid_basic", scene, {0, 15, 30}) && all_ok;
    }

    // 2. shape_transform
    {
        auto scene = Composition("shape_transform")
            .size(256, 256)
            .duration(1.0)
            .fps(30)
            .clear(ColorSpec{0, 0, 0, 255})
            .layer("transform_solid", [](LayerBuilder& l) {
                l.solid("shapes").color({240, 100, 50, 255})
                 .position(128, 128).anchor(64, 64).size(128, 128)
                 .opacity(anim::lerp(1.0, 0.2, 1.0))
                 .duration(1.0);
            })
            .build_scene();

        all_ok = verify_golden_sequence("shape_transform", scene, {0, 15, 30}) && all_ok;
    }

    // 3. text_basic
    {
        auto scene = Composition("text_basic")
            .size(512, 128)
            .duration(1.0)
            .fps(30)
            .clear(ColorSpec{240, 240, 240, 255})
            .layer("text_layer", [](LayerBuilder& l) {
                l.position(256, 64)
                 .text("Tachyon").font("default", 64.0)
                 .color({20, 20, 20, 255})
                 .done()
                 .size(512, 128)
                 .duration(1.0);
            })
            .build_scene();

        all_ok = verify_golden_sequence("text_basic", scene, {0, 15, 30}) && all_ok;
    }

    // 4. mask_basic
    {
        auto scene = Composition("mask_basic")
            .size(256, 256)
            .duration(1.0)
            .fps(30)
            .clear(ColorSpec{10, 10, 10, 255})
            .layer("bg_solid", [](LayerBuilder& l) {
                l.solid("bg").color({80, 80, 200, 255}).size(256, 256).duration(1.0);
            })
            .layer("mask_layer", [](LayerBuilder& l) {
                l.solid("mask").color({255, 255, 255, 255})
                 .position(64, 64).size(128, 128).duration(1.0)
                 .track_matte("bg_solid", TrackMatteType::Alpha);
            })
            .build_scene();

        all_ok = verify_golden_sequence("mask_basic", scene, {0, 15, 30}) && all_ok;
    }

    // 5. precomp_basic
    {
        SceneBuilder sb;
        sb.composition("sub_comp", [](CompositionBuilder& cb) {
            cb.size(128, 128).clear(ColorSpec{255, 128, 0, 255})
              .layer("inner_solid", [](LayerBuilder& l) {
                  l.solid("inner").color({255, 255, 255, 255}).position(32, 32).size(64, 64).duration(1.0);
              });
        });
        sb.composition("root", [](CompositionBuilder& cb) {
            cb.size(256, 256).clear(ColorSpec{0, 0, 0, 255})
              .precomp_layer("nested", "sub_comp", [](LayerBuilder& l) {
                  l.position(64, 64).size(128, 128).duration(1.0);
              });
        });
        auto scene = sb.build();

        all_ok = verify_golden_sequence("precomp_basic", scene, {0, 15, 30}) && all_ok;
    }

    // 6. motion_blur_basic
    {
        AnimatedVector2Spec pos_anim;
        Vector2KeyframeSpec kf0, kf1;
        kf0.time = 0.0;
        kf0.value = {0.0f, 128.0f};
        kf1.time = 1000.0;
        kf1.value = {200.0f, 128.0f};
        pos_anim.keyframes = {kf0, kf1};

        auto scene = Composition("motion_blur_basic")
            .size(256, 256)
            .duration(1.0)
            .fps(30)
            .clear(ColorSpec{0, 0, 0, 255})
            .layer("moving_solid", [&pos_anim](LayerBuilder& l) {
                l.solid("moving").color({255, 255, 255, 255})
                 .position(pos_anim)
                 .size(50, 50).motion_blur(true).duration(1.0);
            })
            .build_scene();

        all_ok = verify_golden_sequence("motion_blur_basic", scene, {0, 8, 16}) && all_ok;
    }

    // 7. transition_fade
    {
        auto scene = Composition("transition_fade")
            .size(256, 256)
            .duration(1.0)
            .fps(30)
            .clear(ColorSpec{30, 30, 30, 255})
            .layer("fading_solid", [](LayerBuilder& l) {
                l.solid("fade").color({200, 40, 100, 255})
                 .position(64, 64).size(128, 128).duration(1.0)
                 .enter().id("fade").duration(0.5).done();
            })
            .build_scene();

        all_ok = verify_golden_sequence("transition_fade", scene, {0, 15, 30}) && all_ok;
    }

    // 8. transition_light_leak
    {
        auto scene = Composition("transition_light_leak")
            .size(256, 256)
            .duration(1.0)
            .fps(30)
            .clear(ColorSpec{0, 0, 0, 255})
            .layer("transitioning_solid", [](LayerBuilder& l) {
                l.solid("leak").color({100, 200, 255, 255})
                 .position(0, 0).size(256, 256).duration(1.0)
                 .enter().id("tachyon.transition.light_leak").duration(0.8).done();
            })
            .build_scene();

        all_ok = verify_golden_sequence("transition_light_leak", scene, {0, 15, 30}) && all_ok;
    }

    // 9. image_layer_basic
    {
        auto scene = Composition("image_layer_basic")
            .size(256, 256)
            .duration(1.0)
            .fps(30)
            .clear(ColorSpec{50, 50, 50, 255})
            .layer("static_background", [](LayerBuilder& l) {
                l.solid("background").color({120, 180, 220, 255})
                 .position(32, 32).size(192, 192).duration(1.0);
            })
            .build_scene();

        all_ok = verify_golden_sequence("image_layer_basic", scene, {0, 15, 30}) && all_ok;
    }

    // 10. blend_modes_basic
    {
        auto scene = Composition("blend_modes_basic")
            .size(256, 256)
            .duration(1.0)
            .fps(30)
            .clear(ColorSpec{128, 128, 128, 255})
            .layer("base_solid", [](LayerBuilder& l) {
                l.solid("base").color({200, 50, 50, 255}).position(32, 32).size(192, 192).duration(1.0);
            })
            .layer("overlay_solid", [](LayerBuilder& l) {
                l.solid("overlay").color({100, 150, 200, 255})
                 .position(64, 64).size(128, 128).blend_mode(BlendMode::Multiply).duration(1.0);
            })
            .build_scene();

        all_ok = verify_golden_sequence("blend_modes_basic", scene, {0, 15, 30}) && all_ok;
    }

    if (all_ok) {
        std::cout << "[SUCCESS] All Extended Visual Regression Sequence Tests passed!\n";
    } else {
        std::cerr << "[ERROR] Some Extended Visual Regression Sequence Tests failed!\n";
    }
    return all_ok;
}

} // namespace tachyon::test
