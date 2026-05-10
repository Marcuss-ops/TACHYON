#include "tachyon/scene/builder.h"
#include "tachyon/core/spec/schema/objects/scene_spec.h"
#include <vector>
#include <string>

using namespace tachyon;
using namespace tachyon::scene;

extern "C" __declspec(dllexport) SceneSpec build_scene() {
    SceneBuilder scene;
    
    scene.composition("main", [](CompositionBuilder& comp) {
        std::vector<std::string> transitions = {
            "tachyon.transition.crossfade",
            "tachyon.transition.wipe_linear",
            "tachyon.transition.slide",
            "tachyon.transition.circle_iris",
            "tachyon.transition.soft_zoom_blur",
            "tachyon.transition.flash_cut",
            "tachyon.transition.smooth_wipe",
            "tachyon.transition.push_left"
        };

        const float static_duration = 1.5f;
        const float trans_duration = 0.8f;
        const int num_steps = static_cast<int>(transitions.size());
        
        comp.size(1920, 1080)
            .fps(60)
            .duration((static_duration + trans_duration) * num_steps + static_duration);

        for (int i = 0; i <= num_steps; ++i) {
            float start_time = i * (static_duration + trans_duration);
            float end_time = start_time + static_duration + (i < num_steps ? trans_duration : 0.0f);
            
            bool is_red = (i % 2 == 0);
            std::string label = is_red ? "Scena A" : "Scena B";
            std::string font = is_red ? "Impact" : "Courier New";
            ColorSpec color = is_red ? ColorSpec{235, 100, 110, 255} : ColorSpec{100, 160, 230, 255};
            
            std::string layer_id = "step_" + std::to_string(i);

            comp.layer(layer_id + "_solid", [=](LayerBuilder& l) {
                l.solid("fill")
                 .color(color)
                 .in(start_time)
                 .out(end_time);
                
                if (i > 0) {
                    l.enter().id(transitions[i-1]).duration(trans_duration);
                }
                if (i < num_steps) {
                    l.exit().id(transitions[i]).duration(trans_duration);
                }
            });

            comp.layer(layer_id + "_text", [=](LayerBuilder& l) {
                l.text(label)
                 .font(font, 200)
                 .color(255, 255, 255, 255)
                 .centerText()
                 .done()
                 .in(start_time)
                 .out(end_time);

                if (i > 0) {
                    l.enter().id(transitions[i-1]).duration(trans_duration);
                }
                if (i < num_steps) {
                    l.exit().id(transitions[i]).duration(trans_duration);
                }
            });
        }
    });

    return scene.build();
}
