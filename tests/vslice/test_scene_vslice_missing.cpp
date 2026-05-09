#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"

extern "C" tachyon::SceneSpec build_scene() {
    using namespace tachyon;
    return scene::Scene()
        .project("vslice_valid_minimal", "Minimal Valid Demo")
        .composition("main", [](scene::CompositionBuilder& c) {
            c.size(1280, 720).fps(30).duration(1.0);
            c.layer(presets::text::headline("MINIMAL DEMO").center().build());
        })
        .build();
}
