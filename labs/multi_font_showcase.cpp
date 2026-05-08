#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"

using namespace tachyon;
using namespace tachyon::scene;

/**
 * @brief Multi-Font Showcase
 * Demonstrates various fonts, all perfectly centered on a black background.
 * 
 * To render:
 * .\build\src\RelWithDebInfo\tachyon.exe render --cpp labs/multi_font_showcase.cpp --out output/multi_font_showcase.mp4
 */
extern "C" __declspec(dllexport) void build_scene(tachyon::SceneSpec& scene) {
    presets::EffectPresetRegistry effects;

    auto build_centered_phrase = [](const std::string& text, const std::string& font, double in, double out) {
        return [text, font, in, out](LayerBuilder& l) {
            auto spec = tachyon::presets::text::title(text)
                .font(font)
                .font_size(120)
                .color({255, 255, 255, 255})
                .center()
                .text_box(1920, 1080)
                .position(960, 540)
                .animate(tachyon::presets::text::kind_fade_up(20.0, 0.6))
                .build();
            spec.id = "text_" + font + "_" + std::to_string(in);
            l.from_spec(spec);
            l.in(in).out(out);
            l.exit().id("fade").duration(0.5).done();
        };
    };

    auto main_comp = Composition("main", effects)
        .size(1920, 1080)
        .duration(10.0)
        .fps(30)
        .layer("bg", [](LayerBuilder& l) { l.solid("black").color({0,0,0,255}).in(0).out(10); })
        .layer("f1", build_centered_phrase("SF PRO (Apple)", "SFPro", 0.0, 2.0))
        .layer("f2", build_centered_phrase("BAHNSCHRIFT (Modern)", "Bahnschrift", 2.0, 4.0))
        .layer("f3", build_centered_phrase("CALIBRI (Soft)", "Calibri", 4.0, 6.0))
        .layer("f4", build_centered_phrase("CONSOLAS (Mono)", "Consolas", 6.0, 8.0))
        .layer("f5", build_centered_phrase("ARIAL (Classic)", "Arial", 8.0, 10.0))
        .build();

    scene.project.name = "Multi-Font Showcase";
    scene.compositions.push_back(std::move(main_comp));
}
