#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"

using namespace tachyon;
using namespace tachyon::scene;

/**
 * @brief Font Showcase Scene
 * Demonstrates the new SF Pro font on a black background with various animations.
 * 
 * To render:
 * .\build\src\RelWithDebInfo\tachyon.exe render --cpp labs/font_showcase.cpp --out output/font_showcase.mp4
 */
extern "C" __declspec(dllexport) void build_scene(tachyon::SceneSpec& scene) {
    presets::EffectPresetRegistry effects;

    auto main_comp = Composition("main", effects)
        .size(1920, 1080)
        .duration(6.0)
        .fps(30)
        // Background layer
        .layer("background", [](LayerBuilder& l) {
            l.solid("black_bg")
             .color({0, 0, 0, 255})
             .in(0).out(6.0);
        })
        // Headline with Fade Up
        .layer("headline_layer", [](LayerBuilder& l) {
            auto spec = tachyon::presets::text::title("TACHYON ENGINE")
                .font("SFPro")
                .font_size(140)
                .color({255, 255, 255, 255})
                .center()
                .position(960, 480)
                .animate(tachyon::presets::text::kind_fade_up(20.0, 0.8))
                .build();
            spec.id = "headline_text";
            l.from_spec(spec);
            l.in(0.5).out(5.5);
        })
        // Subtitle with Tracking Reveal
        .layer("subtitle_layer", [](LayerBuilder& l) {
            auto spec = tachyon::presets::text::subtitle("Premium Typography System")
                .font("SFPro")
                .font_size(60)
                .color({180, 180, 180, 255})
                .center()
                .position(960, 600)
                .animate(tachyon::presets::text::kind_tracking_reveal(30.0, 1.2))
                .build();
            spec.id = "subtitle_text";
            l.from_spec(spec);
            l.in(1.5).out(5.5);
        })
        // Small caption at the bottom
        .layer("caption_layer", [](LayerBuilder& l) {
            auto spec = tachyon::presets::text::caption("Featuring Apple's SF Pro Display Bold")
                .font("SFPro")
                .font_size(32)
                .color({100, 100, 100, 255})
                .center()
                .position(960, 950)
                .animate(tachyon::presets::text::kind_blur_to_focus(10.0, 1.0))
                .build();
            spec.id = "caption_text";
            l.from_spec(spec);
            l.in(2.5).out(5.5);
        })
        .build();

    scene.project.name = "Font Showcase";
    scene.compositions.push_back(std::move(main_comp));
}
