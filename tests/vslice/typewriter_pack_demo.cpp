#include "tachyon/scene/builder.h"
#include "tachyon/text/animation/text_presets.h"

extern "C" void build_scene(tachyon::SceneSpec& out) {
    using namespace tachyon;

    out = scene::Composition("main")
        .size(1920, 1080)
        .fps(24)
        .duration(6.0)
        .clear({11, 15, 20, 255})
        .layer("title", [](scene::LayerBuilder& l) {
            l.text("TYPEWRITER PACK")
             .font("Arial")
             .font_size(120)
             .position(160.0, 260.0)
             .color({245, 248, 255, 255})
             .text_animator(tachyon::text::make_typewriter_minimal_animator(18.0, true));
        })
        .build_scene();
}
