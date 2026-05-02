#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"
#include "tachyon/presets/background/fluent.h"

extern "C" void build_scene(tachyon::SceneSpec& out) {
    using namespace tachyon;
    out = scene::SceneBuilder()
        .project("vslice_60fps", "60 FPS Test")
        .composition("main", [](scene::CompositionBuilder& c) {
            c.size(1920, 1080).fps(60).duration(10.0)
             .background(BackgroundSpec::from_string("#000000"));
             
            c.layer(presets::text::headline("60 FPS STABILITY TEST")
                .font_size(100).center().build());
        }).build();
}
