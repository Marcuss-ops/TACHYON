#include "tachyon/authoring/scene_builder.h"
#include <iostream>

using namespace tachyon::authoring;

int main() {
    auto scene = SceneBuilder()
        .Project("modern_demo", "Modern Tachyon Demo")
        .Composition("Main", 1920, 1080, 10.0)
            .Sequence(0, 5, [](CompositionBuilder& b) {
                b.Layer("solid", "background")
                    .Name("BG")
                    .Opacity(1.0)
                    .Position(960, 540);
                
                b.Stagger(0.2, 5, [](std::size_t i, CompositionBuilder& sb) {
                    sb.Layer("shape", "box_" + std::to_string(i))
                        .Name("Box " + std::to_string(i))
                        .Position(200 + i * 300, 540)
                        .Expression("opacity", interpolate(0, 1, "sin(t)"))
                        .Expression("scale", spring("t", 0, 1, 5, 0.5));
                });
            })
            .Sequence(5, 5, [](CompositionBuilder& b) {
                 b.Layer("text", "title")
                    .Name("Title")
                    .Position(960, 540)
                    .Expression("opacity", spring("t - 5", 0, 1, 2, 0.7));
            })
        .Build();

    std::cout << "Successfully built scene: " << scene.project.name << "\n";
    std::cout << "Composition Main has " << scene.compositions[0].layers.size() << " layers.\n";
    
    // In a real app, we would now pass this to SceneCompiler.
    return 0;
}
