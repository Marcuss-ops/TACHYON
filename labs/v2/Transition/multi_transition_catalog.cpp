#include "tachyon/scene/builder.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <vector>
#include <string>

using namespace tachyon;
using namespace tachyon::scene;

void add_transition_composition(SceneBuilder& scene, const std::string& transition_id) {
    std::string name = transition_id;
    size_t last_dot = name.find_last_of('.');
    if (last_dot != std::string::npos) name = name.substr(last_dot + 1);

    scene.composition(name, [=](CompositionBuilder& comp) {
        comp.size(1920, 1080)
            .fps(30)
            .duration(4.0);

        // Scena A (0-2s)
        comp.layer("scena_a_solid", [=](LayerBuilder& l) {
            l.solid("fill")
             .color({235, 100, 110, 255})
             .in(0.0)
             .out(2.0);
            
            l.exit().id(transition_id).duration(0.8);
        });

        comp.layer("scena_a_text", [=](LayerBuilder& l) {
            l.text("Scena A")
             .font("Impact", 200)
             .color(255, 255, 255, 255)
             .centerText()
             .done()
             .in(0.0)
             .out(2.0);

            l.exit().id(transition_id).duration(0.8);
        });

        // Scena B (2-4s)
        comp.layer("scena_b_solid", [=](LayerBuilder& l) {
            l.solid("fill")
             .color({100, 160, 230, 255})
             .in(1.2) // Overlap for transition
             .out(4.0);
            
            l.enter().id(transition_id).duration(0.8);
        });

        comp.layer("scena_b_text", [=](LayerBuilder& l) {
            l.text("Scena B")
             .font("Courier New", 200)
             .color(255, 255, 255, 255)
             .centerText()
             .done()
             .in(1.2)
             .out(4.0);

            l.enter().id(transition_id).duration(0.8);
        });
    });
}

extern "C" __declspec(dllexport) SceneSpec build_scene() {
    SceneBuilder scene;
    
    std::vector<std::string> transitions = {
        "tachyon.transition.crossfade"
    };

    for (const auto& t : transitions) {
        add_transition_composition(scene, t);
    }

    return scene.build();
}
