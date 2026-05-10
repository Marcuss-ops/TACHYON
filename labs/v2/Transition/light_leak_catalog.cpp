#include "tachyon/scene/builder.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <vector>
#include <string>

using namespace tachyon;
using namespace tachyon::scene;

void add_light_leak_composition(SceneBuilder& scene, const std::string& transition_id) {
    std::string name = transition_id;
    size_t last_dot = name.find_last_of('.');
    if (last_dot != std::string::npos) name = name.substr(last_dot + 1);

    scene.composition(name, [id = transition_id, name](CompositionBuilder& comp) {
        comp.size(1280, 720)
            .fps(30)
            .duration(8.0);
        
        // 1. Scene A (Bottom): Vibrant Red
        comp.layer("scena_a", [](LayerBuilder& l) {
            l.solid("fill")
             .color({235, 100, 110, 255})
             .in(0.0)
             .out(6.0);
            
            l.exit().id("tachyon.transition.crossfade").duration(2.0);
        });

        // 2. Scene B (Middle): Vibrant Blue (Fades in)
        comp.layer("scena_b", [](LayerBuilder& l) {
            l.solid("fill")
             .color({100, 160, 230, 255})
             .in(4.0)
             .out(8.0);
            
            l.enter().id("tachyon.transition.crossfade").duration(2.0);
        });

        // 3. Light Leak Overlay (Top): Flashes over everything
        comp.layer("light_leak_overlay", [id](LayerBuilder& l) {
            l.solid("transparent_base")
             .color({0, 0, 0, 0}) // Fully transparent
             .in(4.0)
             .out(6.0);
            
            l.enter().id(id).duration(1.0); // Flash in
            l.exit().id(id).duration(1.0);  // Flash out
        });

        // Title text
        comp.layer("title", [name](LayerBuilder& l) {
            l.text("label")
             .content(name)
             .font_size(60.0)
             .color(255, 255, 255, 255)
             .align(HorizontalAlign::Center)
             .done()
             .position(640, 680)
             .in(0.0)
             .out(8.0);
        });
    });
}

extern "C" __declspec(dllexport) SceneSpec build_scene() {
    SceneBuilder scene;
    
    std::vector<std::string> light_leaks = {
        "tachyon.transition.light_leak",
        "tachyon.transition.light_leak_solar",
        "tachyon.transition.light_leak_nebula",
        "tachyon.transition.film_burn"
    };

    for (const auto& leak : light_leaks) {
        add_light_leak_composition(scene, leak);
    }

    return scene.build();
}
