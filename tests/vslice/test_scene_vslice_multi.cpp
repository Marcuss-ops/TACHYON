#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"

extern "C" void build_scene(tachyon::SceneSpec& out) {
    using namespace tachyon;
    out = scene::SceneBuilder()
        .project("vslice_multi", "Multi Composition Test")
        .composition("comp1", [](scene::CompositionBuilder& c) {
            c.size(1280, 720).duration(2.0);
            c.layer(presets::text::headline("COMPOSITION ONE").center().build());
        })
        .composition("comp2", [](scene::CompositionBuilder& c) {
            c.size(640, 360).duration(2.0);
            c.layer(presets::text::headline("COMPOSITION TWO").center().build());
        }).build();
}
