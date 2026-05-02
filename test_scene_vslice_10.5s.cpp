#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"
#include "tachyon/presets/background/fluent.h"

extern "C" void build_scene(tachyon::SceneSpec& out) {
    using namespace tachyon;
    out = scene::SceneBuilder()
        .project("vslice_10.5s", "10.5 Seconds Test")
        .composition("main", [](scene::CompositionBuilder& c) {
            c.size(1920, 1080).fps(30).duration(10.5)
             .background(BackgroundSpec::from_string("#000000"));
             
            c.layer(presets::text::headline("10.5 SECONDS TEST")
                .size(100).center().build());
        }).build();
}
