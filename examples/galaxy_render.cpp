#include "tachyon/scene/builder.h"
#include "tachyon/presets/background/background_preset_registry.h"

extern "C" void build_scene(tachyon::SceneSpec& out) {
    using namespace tachyon;
    using namespace tachyon::scene;
    
    out = SceneBuilder()
        .project("galaxy_export", "Galaxy Export")
        .composition("main", [](CompositionBuilder& c) {
            c.size(1920, 1080)
             .fps(30)
             .duration(5.0);
             
            auto& registry = presets::BackgroundPresetRegistry::instance();
            auto layer = registry.create("galaxy_premium", 1920, 1080, 5.0);
            if (layer) {
                c.layer(*layer);
            }
        })
        .build();
}
