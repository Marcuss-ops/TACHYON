#include "tachyon/scene/builder.h"
#include "tachyon/presets/background/procedural.h"
#include "tachyon/core/ids/builtin_ids.h"

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) void build_scene(tachyon::SceneSpec& scene) {
    presets::EffectPresetRegistry effects;
    
    auto comp = Composition("transition_demo", effects)
        .size(1920, 1080)
        .duration(3.0)
        .fps(30);

    // --- SCENE A (Behind) ---
    {
        LayerSpec l;
        l.id = "bg_a";
        l.type = LayerType::Procedural;
        l.width = 1920;
        l.height = 1080;
        l.in_point = 0;
        l.out_point = 2.0;
        ProceduralSpec ps;
        ps.kind = "tachyon.background.kind.aura";
        ps.color_a = AnimatedColorSpec{ColorSpec{255, 0, 0, 255}}; // PURE RED
        ps.color_b = AnimatedColorSpec{ColorSpec{255, 255, 0, 255}}; // YELLOW
        ps.color_c = AnimatedColorSpec{ColorSpec{0, 0, 0, 255}};
        l.procedural = ps;
        comp.layer(l);
    }

    comp.layer("text_a", [&](LayerBuilder& l) {
        l.text().content("SCENE A").font("SFPro").font_size(300).box(1920, 500).align(HorizontalAlign::Center).valign(VerticalAlign::Middle).done()
         .position(960, 540).in(0).out(2.0);
    });

    // --- SCENE B (Front, Transitions In) ---
    {
        LayerSpec l;
        l.id = "bg_b";
        l.type = LayerType::Procedural;
        l.width = 1920;
        l.height = 1080;
        l.in_point = 1.0;
        l.out_point = 3.0;
        ProceduralSpec ps;
        ps.kind = "tachyon.background.kind.aura";
        ps.color_a = AnimatedColorSpec{ColorSpec{0, 0, 255, 255}}; // PURE BLUE
        ps.color_b = AnimatedColorSpec{ColorSpec{0, 255, 255, 255}}; // CYAN
        ps.color_c = AnimatedColorSpec{ColorSpec{20, 20, 20, 255}};
        l.procedural = ps;
        
        l.transition_in.transition_id = "tachyon.transition.zoom";
        l.transition_in.type = "tachyon.transition.zoom";
        l.transition_in.duration = 1.0;
        l.transition_in.easing = animation::EasingPreset::None;
        
        comp.layer(l);
    }

    comp.layer("text_b", [&](LayerBuilder& l) {
        l.text().content("SCENE B").font("SFPro").font_size(300).box(1920, 500).align(HorizontalAlign::Center).valign(VerticalAlign::Middle).done()
         .position(960, 540).in(1.0).out(3.0);
         
        l.enter().id("tachyon.transition.zoom").duration(1.0).done();
    });

    // Header Label
    comp.layer("label", [&](LayerBuilder& l) {
        l.text().content("ZOOM").font("SFPro").font_size(80).box(1920, 150).align(HorizontalAlign::Center).done()
         .position(960, 100).in(0).out(3.0);
    });

    scene.compositions.push_back(comp.build());
}

