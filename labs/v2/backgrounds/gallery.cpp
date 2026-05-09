#include "tachyon/scene/builder.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/text/fluent.h"
#include <vector>
#include <string>

using namespace tachyon;
using namespace tachyon::scene;
using namespace tachyon::presets;

// --- V4: HIGH VIBRANCY, ZERO GRAIN, ULTRA LIGHTWEIGHT ---

inline ProceduralSpec make_v4_aura(ColorSpec a, ColorSpec b, ColorSpec c, float freq = 3.5f) {
    ProceduralSpec spec;
    spec.kind = "tachyon.background.kind.aura";
    spec.color_a = AnimatedColorSpec{a};
    spec.color_b = AnimatedColorSpec{b};
    spec.color_c = AnimatedColorSpec{c};
    spec.frequency = AnimatedScalarSpec{freq};
    spec.amplitude = AnimatedScalarSpec{1.0};
    spec.speed = AnimatedScalarSpec{0.5};
    spec.warp_strength = AnimatedScalarSpec{5.0};
    spec.warp_frequency = AnimatedScalarSpec{2.5};
    spec.grain_amount = AnimatedScalarSpec{0.0}; 
    spec.contrast = AnimatedScalarSpec{1.2};
    spec.softness = AnimatedScalarSpec{0.0};
    return spec;
}

extern "C" __declspec(dllexport) void build_scene(tachyon::SceneSpec& scene) {
    presets::EffectPresetRegistry effects;
    
    struct Item { std::string label; ProceduralSpec spec; };
    std::vector<Item> showcase = {
        {"CYBER NEON", make_v4_aura({10, 0, 30, 255}, {0, 255, 255, 255}, {255, 0, 255, 255}, 4.0f)},
        {"ELECTRIC OCEAN", make_v4_aura({0, 40, 80, 255}, {0, 255, 150, 255}, {0, 100, 255, 255}, 3.0f)},
        {"SOLAR FLARE", make_v4_aura({60, 10, 0, 255}, {255, 200, 0, 255}, {255, 50, 0, 255}, 3.5f)},
        {"MAGENTA MIST", make_v4_aura({40, 0, 40, 255}, {255, 0, 120, 255}, {100, 0, 200, 255}, 4.5f)},
        {"AURORA BOREALIS", make_v4_aura({5, 20, 10, 255}, {0, 255, 100, 255}, {100, 100, 255, 255}, 3.8f)},
        {"PRISM LIGHT", make_v4_aura({200, 240, 255, 255}, {255, 200, 240, 255}, {240, 255, 200, 255}, 5.0f)},
        {"ROYAL GOLD", make_v4_aura({30, 10, 40, 255}, {255, 215, 0, 255}, {150, 100, 0, 255}, 2.5f)}
    };

    const float duration_per_bg = 5.0f;
    auto main_comp = Composition("gallery", effects)
        .size(1920, 1080)
        .duration(duration_per_bg * showcase.size())
        .fps(30);

    for (size_t i = 0; i < showcase.size(); ++i) {
        float start = i * duration_per_bg;
        float end = (i + 1) * duration_per_bg;
        auto& item = showcase[i];

        LayerSpec bg;
        bg.id = "bg_" + std::to_string(i);
        bg.type = LayerType::Procedural;
        bg.procedural = item.spec;
        bg.in_point = start;
        bg.out_point = end;
        main_comp.layer(bg);

        main_comp.layer("label_" + std::to_string(i), [item, start, end](LayerBuilder& l) {
            l.text()
             .content(item.label)
             .font("SFPro")
             .font_size(120)
             .box(1920, 200, TextBoxMode::Fixed)
             .align(HorizontalAlign::Center)
             .valign(VerticalAlign::Middle)
             .fill(255, 255, 255, 220)
             .done()
             .anchor(960, 100)
             .position(960, 920)
             .in(start).out(end);
        });
    }

    scene.compositions.push_back(main_comp.build());
}
