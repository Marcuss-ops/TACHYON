#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"

using namespace tachyon;
using namespace tachyon::scene;

/**
 * @brief User-defined scene script for transition testing.
 * 
 * To render:
 * .\build\src\RelWithDebInfo\tachyon.exe render --cpp labs/transition_demo.cpp --out output/transition_demo.mp4
 */
extern "C" __declspec(dllexport) void build_scene(tachyon::SceneSpec& scene) {
    presets::EffectPresetRegistry effects;

    auto main_comp = Composition("main", effects)
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        // Scena A (Red)
        .layer("scena_a_bg", [](LayerBuilder& l) {
            const char* env_id = std::getenv("TACHYON_TRANSITION_ID");
            std::string trans_id = env_id ? env_id : "slide";

            l.solid("red_bg")
             .color({204, 25, 25, 255})
             .in(0).out(3.0);
            l.exit().id(trans_id).duration(1.0).done();
        })
        .layer("scena_a_text", [](LayerBuilder& l) {
            const char* env_id = std::getenv("TACHYON_TRANSITION_ID");
            std::string trans_id = env_id ? env_id : "slide";

            auto spec = tachyon::presets::text::headline("SCENA A")
                .font("SFPro")
                .font_size(120)
                .color({255, 255, 255, 255})
                .center()
                .text_box(1920, 1080)
                .position(960, 540)
                .build();
            spec.id = "scena_a_text";
            l.from_spec(spec);
            l.in(0).out(3.0);
            l.exit().id(trans_id).duration(1.0).done();
        })
        // Scena B (Blue)
        .layer("scena_b_bg", [](LayerBuilder& l) {
            const char* env_id = std::getenv("TACHYON_TRANSITION_ID");
            std::string trans_id = env_id ? env_id : "slide";

            l.solid("blue_bg")
             .color({25, 25, 204, 255})
             .in(2.0).out(5.0);
            l.enter().id(trans_id).duration(1.0).done();
        })
        .layer("scena_b_text", [](LayerBuilder& l) {
            const char* env_id = std::getenv("TACHYON_TRANSITION_ID");
            std::string trans_id = env_id ? env_id : "slide";

            auto spec = tachyon::presets::text::headline("SCENA B")
                .font("SFPro")
                .font_size(120)
                .color({255, 255, 255, 255})
                .center()
                .text_box(1920, 1080)
                .position(960, 540)
                .build();
            spec.id = "scena_b_text";
            l.from_spec(spec);
            l.in(2.0).out(5.0);
            l.enter().id(trans_id).duration(1.0).done();
        })
        .build();

    scene.project.name = "Transition Demo (Labs)";
    scene.compositions.push_back(std::move(main_comp));
}
