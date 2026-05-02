#include "tachyon/scene/builder.h"
#include "tachyon/presets/text/fluent.h"
#include "tachyon/presets/background/fluent.h"
#include "tachyon/presets/background/procedural.h"

extern "C" void build_scene(tachyon::SceneSpec& out) {
    using namespace tachyon;
    
    out = scene::SceneBuilder()
        .project("test_vslice", "Vertical Slice Test")
        .composition("main", [](scene::CompositionBuilder& c) {
            c.size(1920, 1080)
             .fps(30)
             .duration(10.0)
             .background(BackgroundSpec::from_string("#0d1117"));
             
            // Animated background
            c.layer(presets::background::aura()
                .width(1920)
                .height(1080)
                .duration(10.0)
                .palette(presets::background::procedural_bg::palettes::neon_night())
                .grain(0.1)
                .build());
             
            // Title text
            c.layer(presets::text::headline("TACHYON FULL HD TEST")
                .font_size(120)
                .center()
                .color({255, 255, 255, 255})
                .build());
        })
        .build();
}
