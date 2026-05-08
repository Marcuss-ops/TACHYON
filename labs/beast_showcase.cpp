#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"

using namespace tachyon;
using namespace tachyon::scene;

/**
 * @brief Beast Showcase Scene
 * Demonstrates a sequence of high-impact phrases with transitions and the new SF Pro font.
 * 
 * To render:
 * .\build\src\RelWithDebInfo\tachyon.exe render --cpp labs/beast_showcase.cpp --out output/beast_showcase.mp4
 */
extern "C" __declspec(dllexport) void build_scene(tachyon::SceneSpec& scene) {
    presets::EffectPresetRegistry effects;

    auto build_phrase = [](const std::string& text, double in, double out, const std::string& enter_trans, const std::string& exit_trans) {
        return [text, in, out, enter_trans, exit_trans](LayerBuilder& l) {
            auto spec = tachyon::presets::text::title(text)
                .font("SFPro")
                .font_size(120)
                .color({255, 255, 255, 255})
                .center()
                .position(960, 540)
                .build();
            spec.id = "text_" + text;
            l.from_spec(spec);
            l.in(in).out(out);
            
            if (!enter_trans.empty()) {
                l.enter().id(enter_trans).duration(0.8).done();
            }
            if (!exit_trans.empty()) {
                l.exit().id(exit_trans).duration(0.8).done();
            }
        };
    };

    auto main_comp = Composition("main", effects)
        .size(1920, 1080)
        .duration(10.0)
        .fps(30)
        // Background
        .layer("background", [](LayerBuilder& l) {
            l.solid("black_bg").color({0, 0, 0, 255}).in(0).out(10.0);
        })
        // Sequence of phrases
        .layer("p1", build_phrase("TACHYON ENGINE", 0.0, 2.5, "fade", "slide"))
        .layer("p2", build_phrase("UNLEASH THE POWER", 1.7, 4.5, "slide", "wipe"))
        .layer("p3", build_phrase("MODERN TYPOGRAPHY", 3.7, 6.5, "wipe", "zoom"))
        .layer("p4", build_phrase("ULTRA-FAST RENDERING", 5.7, 8.5, "zoom", "glitch"))
        .layer("p5", build_phrase("THE BEAST IS HERE", 7.7, 10.0, "glitch", "fade"))
        .build();

    scene.project.name = "Beast Showcase";
    scene.compositions.push_back(std::move(main_comp));
}
