#include "tachyon/scene/builder.h"
#include "tachyon/background_registry.h"
#include "tachyon/presets/background/background_builders.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/presets/text/fluent.h"

using namespace tachyon;
using namespace tachyon::scene;
using namespace tachyon::presets::background::procedural_bg;

// Factory for this specific background
inline ProceduralSpec make_this_spec(const ProceduralParams& params) {
    ProceduralSpec spec;
    spec.kind = "tachyon.background.kind.classico_premium";
    spec.seed = params.seed;
    spec.color_a = AnimatedColorSpec{params.palette_a};
    spec.color_b = AnimatedColorSpec{params.palette_b};
    spec.color_c = AnimatedColorSpec{params.palette_c};
    spec.frequency = AnimatedScalarSpec{0.75};
    spec.amplitude = AnimatedScalarSpec{0.65};
    spec.scale = AnimatedScalarSpec{1.25};
    spec.warp_strength = AnimatedScalarSpec{2.5};
    spec.warp_frequency = AnimatedScalarSpec{1.2};
    spec.grain_amount = AnimatedScalarSpec{params.grain > 0.0 ? params.grain : 0.08};
    spec.contrast = AnimatedScalarSpec{params.contrast};
    spec.gamma = AnimatedScalarSpec{1.0};
    spec.saturation = AnimatedScalarSpec{0.9};
    spec.softness = AnimatedScalarSpec{0.35};
    return spec;
}

extern "C" __declspec(dllexport) void build_scene(tachyon::SceneSpec& scene) {
    auto& registry = BackgroundRegistry::instance();
    
    // Register locally
    {
        BackgroundDescriptor desc;
        desc.id = "tachyon.background.kind.premium_dark";
        desc.kind = BackgroundKind::Procedural;
        desc.build = [](const tachyon::registry::ParameterBag& bag) -> LayerSpec {
            ProceduralParams p;
            p.palette_a = bag.get_or<ColorSpec>("palette.a", ColorSpec{10, 10, 10, 255});
            p.palette_b = bag.get_or<ColorSpec>("palette.b", ColorSpec{26, 26, 46, 255});
            p.palette_c = bag.get_or<ColorSpec>("palette.c", ColorSpec{15, 15, 25, 255});
            p.motion_speed = bag.get_or<double>("motion_speed", 0.35);
            p.contrast = bag.get_or<double>("contrast", 1.05);
            p.grain = bag.get_or<double>("grain", 0.08);
            p.softness = bag.get_or<double>("softness", 0.35);
            p.seed = static_cast<uint64_t>(bag.get_or<int>("seed", 12345));

            LayerSpec layer;
            layer.type = LayerType::Procedural;
            layer.procedural = make_this_spec(p);
            return layer;
        };
        registry.register_descriptor(desc);
    }

    presets::EffectPresetRegistry effects;
    auto main_comp = Composition("main", effects)
        .size(1920, 1080)
        .duration(5.0)
        .fps(30)
        .layer("bg", [](LayerBuilder& l) {
            BackgroundParams p;
            p.kind = "tachyon.background.kind.premium_dark";
            p.w = 1920; p.h = 1080;
            p.in_point = 0; p.out_point = 5.0;
            l.custom_layer(build_background(p));
        })
        .layer("label", [](LayerBuilder& l) {
            l.text()
             .content("PREMIUM DARK")
             .font("SFPro")
             .font_size(100)
             .box(1920, 200, TextBoxMode::Fixed)
             .align(HorizontalAlign::Center)
             .valign(VerticalAlign::Middle)
             .color({255, 255, 255, 200})
             .anchor(960, 540)
             .position(960, 540)
             .in(0).out(5.0);
        })
        .build();

    scene.compositions.push_back(std::move(main_comp));
}
