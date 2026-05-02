#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"

extern "C" void build_scene(tachyon::SceneSpec& out) {
    using namespace tachyon;
    out = scene::SceneBuilder()
        .project("vslice_missing", "Missing Asset Test")
        .composition("main", [](scene::CompositionBuilder& c) {
            c.size(1280, 720).duration(1.0);
            c.layer(presets::text::headline("THIS SHOULD FAIL").center().build());
            // Intentionally missing asset
            c.image("assets/non_existent_file_12345.png");
        }).build();
}
