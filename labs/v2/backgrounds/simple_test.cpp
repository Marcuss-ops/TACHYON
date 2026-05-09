#include "tachyon/scene/builder.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/background/procedural.h"

using namespace tachyon;
using namespace tachyon::scene;
using namespace tachyon::presets;

extern "C" __declspec(dllexport) void build_scene(tachyon::SceneSpec& scene) {
    presets::EffectPresetRegistry effects;

    auto main_comp = Composition("main", effects)
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .layer("bg", [](LayerBuilder& l) {
            LayerSpec spec;
            spec.type = LayerType::Procedural;
            
            ProceduralSpec p;
            p.kind = "tachyon.background.kind.aura";
            p.color_a = AnimatedColorSpec{ColorSpec{10, 10, 20, 255}};
            p.color_b = AnimatedColorSpec{ColorSpec{50, 20, 80, 255}};
            p.speed = AnimatedScalarSpec{1.0};
            p.frequency = AnimatedScalarSpec{1.0};
            
            spec.procedural = p;
            spec.in_point = 0;
            spec.out_point = 5.0;
            
            l.from_spec(spec);
        })
        .build();

    scene.compositions.push_back(std::move(main_comp));
}
