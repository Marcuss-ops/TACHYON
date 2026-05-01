#include "tachyon/presets/presets.h"
#include "tachyon/scene/builder.h"
#include <iostream>

using namespace tachyon;
using namespace tachyon::scene;

int main() {
    // Build scene using the new fluent API
    SceneSpec scene = Scene()
        .project("minimal_white_render", "Minimalist White Background")
        .composition("main", [](auto& c) {
            c.size(1920, 1080)
             .fps(30)
             .duration(5.0)
             .layer(background::minimal_white({}));
        })
        .build();

    // Output scene as JSON for rendering
    nlohmann::json j = scene;
    std::cout << j.dump(2) << std::endl;

    return 0;
}
